/* Compile time bound timer lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRLIST)
#define INC_COBTMRLIST


#include "coblist.hpp"


// `B` is the bound of the number of timers.
// With `CHECKED` being `true`, the index of a timer is checked: once that
// timer is not running, it is the index of nothing, also when a new timer
// takes its place.
// With `HIVE` being `true`, which slots hold a timer is kept as a bit each.
template <class ID, class B, class U, bool CHECKED = false, bool HIVE = false> class bndtmrlist {
    struct elem {
        ID id;
        U  remaining;
        elem() {}
        elem(ID id_)
            : id(id_)
        {
        }
        elem(ID id_, U remain)
            : id(id_)
            , remaining(remain)
        {
        }
        bool operator==(elem const& x) const { return id == x.id; }
    };

    bndlist<elem, B, CHECKED, HIVE> timers;

public:
    using tmrID = ID;
    using index = typename bndlist<elem, B, CHECKED, HIVE>::link;

    bool empty() const { return timers.empty(); }

    index start(ID id, U duration)
    {
        auto it = timers.lbegin();
        for (; it != timers.lend(); it = timers.fwd(it)) {
            auto tmr = timers.get(it);
            if (tmr.remaining > duration) {
                break;
            }
            duration -= tmr.remaining;
        }
        const index rslt = timers.insert(it, { id, duration });
        // Only if we're actually in, the one after us waits that much less
        if ((rslt != timers.lend()) && (it != timers.lend())) {
            auto tmr = timers.get(it);
            tmr.remaining -= duration;
            timers.set(it, tmr);
        }
        return rslt;
    }

    int stop(ID id)
    {
        for (auto it = timers.lbegin(); it != timers.lend(); it = timers.fwd(it)) {
            auto tmr = timers.get(it);
            if (tmr.id == id) {
                return stop(it);
            }
        }
        return -1;
    }

    int stop(index it)
    {
        if (timers.live(it)) {
            auto tmr = timers.get(it);
            auto fwd = timers.fwd(it);
            if (fwd != timers.lend()) {
                auto next = timers.get(fwd);
                next.remaining += tmr.remaining;
                timers.set(fwd, next);
            }
            timers.erase(it);
            return 0;
        }
        return -1;
    }

    template <template <class> class V> constexpr V<ID> stop_first()
    {
        if (timers.empty()) {
            return {};
        }
        auto e = timers.get(timers.lfront());
        stop(timers.lfront());
        return e.id;
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        // Always look at the first timer and remove it before reporting,
        // so that `f` can start and stop timers.
        while (!timers.empty()) {
            const auto it  = timers.lbegin();
            auto       tmr = timers.get(it);
            if (tmr.remaining > elapsed) {
                tmr.remaining -= elapsed;
                timers.set(it, tmr);
                break;
            }
            elapsed -= tmr.remaining;
            timers.erase(it);
            f(tmr.id);
        }
    }

    // std::optional<ID> expired(U proteklo) {}

private:
    template <class, class, class, unsigned, bool, bool> friend class bndtmrwheel;
    // For the wheel: the time left for the first timer, if there is one
    U first_remaining() const { return timers.empty() ? U{} : timers.get(timers.lfront()).remaining; }
};

template <class ID, int N, class U> using cobtmrlist        = bndtmrlist<ID, cobbound<N>, U>;
// The one with checked indices
template <class ID, int N, class U> using cobcheckedtmrlist = bndtmrlist<ID, cobbound<N>, U, true>;

// The ones where which slots hold a timer is kept as a bit each (see `cobhive`)
template <class ID, int N, class U>
using cobhivetmrlist = bndtmrlist<ID, cobbound<N>, U, false, true>;
template <class ID, int N, class U>
using cobcheckedhivetmrlist = bndtmrlist<ID, cobbound<N>, U, true, true>;

#endif // define      INC_COBTMRLIST
