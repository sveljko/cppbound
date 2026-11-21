/* Compile time bound timer mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRMILL)
#define INC_COBTMRMILL


#include "cobarray.hpp"
#include "coblist.hpp"
#include "cobpool.hpp"
#include "cobtmrleanmill.hpp"     // for the bound of a spoke

#include <type_traits>


// A timer stays in the spoke it was put in when it was started, so the
// further the level, the less precise it is.
// `S` is the bound of the number of timers of a spoke.
// With `CHECKED` being `true`, the index of a timer is checked: once that
// timer is not running, it is the index of nothing.
// With `HIVE` being `true`, which slots hold a timer is kept as a bit each.
template <class ID, class S, class U, unsigned LVL = 5, unsigned DIM = 64, bool CHECKED = false, bool HIVE = false>
class bndtmrmill {
    static_assert(LVL * DIM > 0, "Mill must have some spokes");

    using level    = cobint<0, LVL - 1>;
    using lvlindex = cobint<0, DIM - 1>;
    using spoke    = bndlist<ID, S, CHECKED, HIVE>;

    spoke                   timers[LVL][DIM];
    spoke                   expired;    // those of the spoke that is being reported
    cobarray<lvlindex, LVL> next;

public:
    using tmrID = ID;

    bndtmrmill() { next.fill(cobic<0>); }

    static constexpr U max_duration{ cobhlp::pow(DIM, LVL) - 1 };

    struct index {
        level                lvl;
        lvlindex             ispoke;
        typename spoke::link idx;

        bool valid() const { return idx != spoke::nil; }
    };

    index start(ID id, U duration)
    {
        index i{ level{}, cobic<0>, spoke::nil };
        if (spoke_from_duration(duration, i.lvl, i.ispoke)) {
            auto& spoke = timers[i.lvl.get()][i.ispoke.get()];
            if (spoke.push_front(id)) {
                i.idx = spoke.lbegin();
            }
        }
        return i;
    }

    int stop(ID id)
    {
        for (auto& l : timers) {
            for (auto& spoke : l) {
                if (spoke.remove(id) > 0) {
                    return 0;
                }
            }
        }
        return -1;
    }

    void stop(index it) { timers[it.lvl.get()][it.ispoke.get()].erase(it.idx); }

    template <template <class> class V> constexpr V<ID> stop_first()
    {
        level lvl;
        do {
            auto ispoke = next.get(lvl);
            do {
                auto& list = timers[lvl.get()][ispoke.get()];
                if (!list.empty()) {
                    ID rslt = list.get(list.lfront());
                    list.erase(list.lfront());
                    return rslt;
                }
                if (!ispoke.advance()) {
                    auto next_lvl = lvl;
                    if (next_lvl.advance()) {
                        auto nxt_lvl_idx = next.get(next_lvl);
                        auto& nxtlist = timers[next_lvl.get()][nxt_lvl_idx.get()];
                        if (!nxtlist.empty()) {
                            ID rslt = nxtlist.get(nxtlist.lfront());
                            nxtlist.erase(nxtlist.lfront());
                            return rslt;
                        }
                    }
                    ispoke = cobic<0>;
                }
            } while (ispoke != next.get(lvl));
        } while (lvl.advance());
        return {};
    }


    template <class F> void process_expired(U elapsed, F f)
    {
        for (U i{ 0 }; i < elapsed; ++i) {
            // First turn the mill and take the expired timers out, then
            // report them, so that `f` can start and stop timers.
            cobarray<lvlindex, LVL> due;
            level                   top;
            level                   lvl;
            do {
                auto ispoke = next.get(lvl);
                due.set(lvl, ispoke);
                top = lvl;
                if (ispoke.advance()) {
                    next.set(lvl, ispoke);
                    break;
                }
                next.set(lvl, cobic<0>);
            } while (lvl.advance());

            lvl = level{};
            do {
                auto& live = timers[lvl.get()][due.get(lvl).get()];
                expired    = live;
                live.clear();
                for (auto const& id : expired) {
                    f(id);
                }
            } while ((lvl != top) && lvl.advance());
        }
    }

    // std::optional<ID> expired(U proteklo) {}

private:
    // false if there is no spoke for that duration
    bool spoke_from_duration(U duration, level& lvl, lvlindex& ispoke) const
    {
        // Before anything else, as the duration might not fit our integer
        if ((duration < U{ 0 }) || (duration > max_duration)) {
            return false;
        }
        unsigned d = static_cast<unsigned>(duration.count());
        lvl        = level{};
        do {
            if (ispoke.be(d)) {
                d = (d + next.get(lvl).get()) % (ispoke.greatest().get() + 1);
                ispoke.be(d);
                return true;
            }
            d = d / DIM;
        } while (lvl.advance());
        return false;
    }
};



// The exact mill, of the same interface: timers are cascaded. When a wheel
// has turned around, the timers of the spoke that is due at the next level
// are put anew, by what is left of their time, so each one ends up at the
// first level and expires precisely. For that, the time it is due at is kept
// with each timer and all the spokes share the room for timers.
// `B` is the bound of the number of timers, and `X` the bound of that many
// and `LVL * DIM` more.
template <class ID, class B, class X, class U, unsigned LVL = 5, unsigned DIM = 64, bool HIVE = false>
class bndtmrexactmill {
    static_assert(LVL > 0, "Mill must have some levels");
    static_assert(DIM > 1, "A wheel must have more than one spoke");

    using tick = unsigned long long;

    // After this many ticks, all the wheels are where they were
    static constexpr tick period = cobhlp::pow(DIM, LVL);
    static_assert((cobhlp::pow(DIM, LVL - 1) <= (~tick{ 0 } >> 2) / DIM), "Too many spokes");

    struct timer {
        ID   id{};
        tick due = 0;       // the tick it expires on, less than `period`
        timer() = default;
        timer(ID const& id_, tick due_) : id(id_), due(due_) {}
    };
    // The handles are checked: the index of a timer that has expired or was
    // stopped is the index of nothing, also when a new timer takes its slot.
    // The mill itself goes by the slots.
    struct pool : bndpool<timer, B, true, HIVE> {
        using base = bndpool<timer, B, true, HIVE>;
        using typename base::node;
        using base::at;
        using base::drop_node;
        using base::live_node;
        using base::make_node;
        using base::raw;
        using base::ticket;
    };
    using handle = typename pool::handle;
    using node   = typename pool::node;
    using slot   = typename B::slot;
    // A timer (its number is that of its slot) or, after all of those,
    // the head of a spoke. Each spoke is a ring: the head, then its timers.
    using link = typename X::slot;

    pool                             timers;
    typename X::template array<link> next;
    typename X::template array<link> prev;
    tick                             now = 0;

public:
    using tmrID = ID;

    bndtmrexactmill()
    {
        for (auto i : X::slots()) {
            next.set(i, i);
            prev.set(i, i);
        }
    }

    static constexpr U max_duration{ period - 1 };

    struct index {
        handle idx;

        bool valid() const { return idx != pool::nil; }
    };

    index start(ID id, U duration)
    {
        // Before anything else, as the duration might not fit our integer
        if ((duration < U{ 0 }) || (duration > max_duration)) {
            return index{ pool::nil };
        }
        // Like the first level of the mill that is not exact: on the tick after the duration
        const tick left = static_cast<tick>(duration.count()) + 1;
        const tick due  = (now + left) % period;
        const node n    = timers.make_node(id, due);
        link       at;
        if ((n != pool::nil) && at.be(n.get())) {
            hook(at, spoke_for(due, left));
        }
        return index{ timers.ticket(n) };
    }

    int stop(ID id)
    {
        for (auto i : B::slots()) {
            if (timers.live_node(i) && (timers.at(i).id == id)) {
                take(i);
                return 0;
            }
        }
        return -1;
    }

    // Does nothing if that timer is not running any more
    void stop(index it)
    {
        slot i;
        if (i.be(timers.raw(it.idx).get())) {
            take(i);
        }
    }

    template <template <class> class V> V<ID> stop_first()
    {
        slot first;
        bool any   = false;
        tick least = 0;
        for (auto i : B::slots()) {
            if (timers.live_node(i)) {
                // Nothing is left only of a timer that is yet to go a whole period
                const tick ahead = (timers.at(i).due + period - now) % period;
                const tick left  = (ahead == 0) ? period : ahead;
                if (!any || (left < least)) {
                    first = i;
                    least = left;
                    any   = true;
                }
            }
        }
        if (!any) {
            return {};
        }
        V<ID> rslt(timers.at(first).id);
        take(first);
        return rslt;
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        for (U i{ 0 }; i < elapsed; ++i) {
            now = (now + 1) % period;
            // Each wheel that has just turned around hands the timers of
            // the spoke that is due at the next level down, to where they
            // belong by what is left of their time
            tick unit = DIM;
            for (unsigned lvl = 1; (lvl < LVL) && (now % unit == 0); ++lvl, unit *= DIM) {
                const link head = spoke(lvl, (now / unit) % DIM);
                link       at   = next.get(head);
                while (at != head) {
                    const link after = next.get(at);
                    const tick due   = due_of(at);
                    unhook(at);
                    hook(at, spoke_for(due, (due + period - now) % period));
                    at = after;
                }
            }
            // Take a timer out, then report it, so that `f` can start and
            // stop timers. What `f` starts never gets to this spoke now.
            const link head = spoke(0, now % DIM);
            slot       t;
            for (link at = next.get(head); (at != head) && t.be(at.get()); at = next.get(head)) {
                const ID id = timers.at(t).id;
                take(t);
                f(id);
            }
        }
    }

private:
    static link spoke(unsigned lvl, tick ispoke)
    {
        link rslt;
        rslt.be(static_cast<decltype(rslt.get())>(B::size() + lvl * DIM + ispoke));
        return rslt;
    }
    // The spoke for a timer due at `due`, which is `left` ticks ahead (at
    // most a period): at the first level whose wheel does not turn around
    // before that, the last level taking all the rest
    static link spoke_for(tick due, tick left)
    {
        unsigned lvl  = 0;
        tick     unit = 1;
        while ((lvl + 1 < LVL) && (left >= unit * DIM)) {
            unit *= DIM;
            ++lvl;
        }
        return spoke(lvl, (due / unit) % DIM);
    }
    tick due_of(link at) const
    {
        slot t;
        return t.be(at.get()) ? timers.at(t).due : tick{ 0 };
    }
    // Out of its spoke, and gone
    void take(slot t)
    {
        link at;
        if (at.be(t.get())) {
            unhook(at);
        }
        timers.drop_node(t);
    }
    // Last in the ring of `head`
    void hook(link at, link head)
    {
        const link last = prev.get(head);
        next.set(last, at);
        prev.set(at, last);
        next.set(at, head);
        prev.set(head, at);
    }
    // Out of its ring, and into one of its own, so that it's fine to do twice
    void unhook(link at)
    {
        next.set(prev.get(at), next.get(at));
        prev.set(next.get(at), prev.get(at));
        next.set(at, at);
        prev.set(at, at);
    }
};

namespace cobhlp {
// The regular and the exact mill of N timers
template <class ID, int N, class U, unsigned LVL, unsigned DIM, bool CHECKED = false, bool HIVE = false>
struct roughmill {
    using type = bndtmrmill<ID, typename millspoke<N, LVL, DIM>::type, U, LVL, DIM, CHECKED, HIVE>;
};
template <class ID, int N, class U, unsigned LVL, unsigned DIM, bool HIVE = false> struct exactmill {
    using type = bndtmrexactmill<ID, cobbound<N>, cobbound<N + static_cast<int>(LVL * DIM)>, U, LVL, DIM, HIVE>;
};
} // namespace cobhlp

// With `EXACT` being `true`, this is the exact mill. Then all N timers can be
// in whatever spokes; otherwise, each spoke has room for its share of them.
template <class ID, int N, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using cobtmrmill = typename std::conditional_t<EXACT, cobhlp::exactmill<ID, N, U, LVL, DIM>,
                                               cobhlp::roughmill<ID, N, U, LVL, DIM>>::type;

// The one with checked indices (which the exact mill has anyway)
template <class ID, int N, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using cobcheckedtmrmill = typename std::conditional_t<EXACT, cobhlp::exactmill<ID, N, U, LVL, DIM>,
                                                      cobhlp::roughmill<ID, N, U, LVL, DIM, true>>::type;

// The ones where which slots hold a timer is kept as a bit each
template <class ID, int N, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using cobhivetmrmill = typename std::conditional_t<EXACT, cobhlp::exactmill<ID, N, U, LVL, DIM, true>,
                                                   cobhlp::roughmill<ID, N, U, LVL, DIM, false, true>>::type;
template <class ID, int N, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using cobcheckedhivetmrmill = typename std::conditional_t<EXACT, cobhlp::exactmill<ID, N, U, LVL, DIM, true>,
                                                          cobhlp::roughmill<ID, N, U, LVL, DIM, true, true>>::type;

#endif // define      INC_COBTMRMILL
