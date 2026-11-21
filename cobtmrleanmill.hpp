/* Compile time bound timer lean mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRLEANMILL)
#define INC_COBTMRLEANMILL


#include "cobarray.hpp"
#include "cobmatrix.hpp"

#include <type_traits>


// `S` is the bound of the number of timers of a spoke.
// With `CHECKED` being `true`, the index of a timer is checked: once the
// spoke that timer was in has expired, it is the index of nothing. That
// takes a counter for each spoke (not for each timer).
template <class ID, class S, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64, bool CHECKED = false>
class bndtmrleanmill {
    static_assert(LVL * DIM > 0, "Mill must have some spokes");

    using level    = cobint<0, LVL - 1>;
    using lvlindex = cobint<0, DIM - 1>;
    using spoke    = typename S::template values<ID>;
    using spokepos = typename S::link;      // `nil`: not a timer

    // The stamp of a spoke changes each time the spoke expires, so the index
    // of a timer, which has the stamp of when it was started, is good while
    // the stamps are the same. If not checked, there is nothing to it.
    struct stamped {
        cobmatrix<unsigned, LVL, DIM> count;
        stamped() { count.fill(0); }
        unsigned of(level lvl, lvlindex ispoke) const { return count.get({ lvl, ispoke }); }
        void     next(level lvl, lvlindex ispoke) { count.set({ lvl, ispoke }, count.get({ lvl, ispoke }) + 1u); }
    };
    struct unstamped {
        struct none {
            bool operator==(none const&) const { return true; }
        };
        none of(level, lvlindex) const { return {}; }
        void next(level, lvlindex) {}
    };
    using stamps = std::conditional_t<CHECKED, stamped, unstamped>;
    using stamp  = std::conditional_t<CHECKED, unsigned, typename unstamped::none>;

    spoke                         timers[LVL][DIM];
    cobmatrix<unsigned, LVL, DIM> active;
    cobarray<lvlindex, LVL>       next;
    [[no_unique_address]] stamps  expiries;

public:
    using tmrID = ID;

    bndtmrleanmill()
    {
        active.fill(0);
        next.fill(cobic<0>);
    }

    static constexpr U max_duration{ cobhlp::pow(DIM, LVL) - 1 };

    struct index {
        level    lvl;
        lvlindex ispoke;
        spokepos idx;
        [[no_unique_address]] stamp when{};

        bool valid() const { return idx != S::nil; }
    };

    constexpr index spoke_from_duration(U duration)
    {
        level lvl;
        // Before anything else, as the duration might not fit our integer
        if ((duration < U{ 0 }) || (duration > max_duration)) {
            return { lvl, cobic<0>, S::nil, {} };
        }
        unsigned d = static_cast<unsigned>(duration.count());
        do {
            lvlindex idx;
            if (idx.be(d)) {
                d = (d + next.get(lvl).get()) % (idx.greatest().get() + 1);
                idx.be(d);
                return { lvl, idx, spokepos{}, {} };
            }
            d = d / DIM;
        } while (lvl.advance());
        return { lvl, cobic<0>, S::nil, {} };
    }

    constexpr index start(ID id, U duration)
    {
        index i = spoke_from_duration(duration);
        if (i.valid()) {
            auto& spoke = timers[i.lvl.get()][i.ispoke.get()];
            auto  pos   = active.get({ i.lvl, i.ispoke });
            if (i.idx.be(pos) && spoke.maybe_set(pos, id)) {
                active.set({ i.lvl, i.ispoke }, pos + 1);
                i.when = expiries.of(i.lvl, i.ispoke);
            }
            else {
                i.idx = S::nil;
            }
        }
        return i;
    }

    int stop(ID id)
    {
        level lvl;
        do {
            lvlindex ispoke;
            do {
                for (unsigned i = 0; i < active.get({ lvl, ispoke }); ++i) {
                    NULIFY nlf;
                    auto   p = timers[lvl.get()][ispoke.get()].begin() + i;
                    if (*p == id) {
                        nlf(*p);
                        return 0;
                    }
                }
            } while (ispoke.advance());
        } while (lvl.advance());
        return -1;
    }

    void stop(index it)
    {
        if (it.valid() && (it.when == expiries.of(it.lvl, it.ispoke))
            && (it.idx.get() < static_cast<decltype(it.idx.get())>(active.get({ it.lvl, it.ispoke })))) {
            NULIFY nlf;
            auto p = timers[it.lvl.get()][it.ispoke.get()].begin() + static_cast<std::ptrdiff_t>(it.idx.get());
            nlf(*p);
        }
    }

    template <template <class> class V> constexpr V<ID> stop_first()
    {
        level lvl;
        do {
            auto ispoke = next.get(lvl);
            do {
                if (const int pos = first_live(lvl, ispoke); pos >= 0) {
                    return nulify_at(lvl, ispoke, pos);
                }
                if (!ispoke.advance()) {
                    auto next_lvl = lvl;
                    if (next_lvl.advance()) {
                        auto nxt_lvl_idx = next.get(next_lvl);
                        if (const int pos = first_live(next_lvl, nxt_lvl_idx); pos >= 0) {
                            return nulify_at(next_lvl, nxt_lvl_idx, pos);
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
            level lvl;
            do {
                auto ispoke = next.get(lvl);
                auto p      = timers[lvl.get()][ispoke.get()].begin();
                for (unsigned j = 0; j < active.get({ lvl, ispoke }); ++j) {
                    f(*p);
                    ++p;
                }
                active.set({ lvl, ispoke }, 0);
                expiries.next(lvl, ispoke);

                if (ispoke.advance()) {
                    next.set(lvl, ispoke);
                    break;
                }
                next.set(lvl, cobic<0>);
            } while (lvl.advance());
        }
    }

    // std::optional<ID> expired(U proteklo) {}

private:
    // Position of the first timer in the spoke that is not nulified, or -1
    constexpr int first_live(level lvl, lvlindex ispoke) const
    {
        auto p = timers[lvl.get()][ispoke.get()].begin();
        for (unsigned i = 0; i < active.get({ lvl, ispoke }); ++i, ++p) {
            ID id = *p;
            if (NULIFY{}(id)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    constexpr ID nulify_at(level lvl, lvlindex ispoke, int pos)
    {
        auto p    = timers[lvl.get()][ispoke.get()].begin() + pos;
        ID   rslt = *p;
        NULIFY{}(*p);
        return rslt;
    }
};

namespace cobhlp {
// The bound of a spoke of a mill of N timers: a share of them, and one more
template <int N, unsigned LVL, unsigned DIM> struct millspoke {
    static_assert(N > 0, "Mill must have some timers");
    static_assert(LVL * DIM < static_cast<unsigned>(N), "Too many spokes");
    using type = cobbound<N / static_cast<int>(LVL * DIM) + 1>;
};
} // namespace cobhlp

template <class ID, int N, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
using cobtmrleanmill = bndtmrleanmill<ID, typename cobhlp::millspoke<N, LVL, DIM>::type, U, NULIFY, LVL, DIM>;

// The one with checked indices
template <class ID, int N, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
using cobcheckedtmrleanmill =
    bndtmrleanmill<ID, typename cobhlp::millspoke<N, LVL, DIM>::type, U, NULIFY, LVL, DIM, true>;

#endif // define      INC_COBTMRLEANMILL
