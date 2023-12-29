/* Compile time bound timer lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRLIST)
#define INC_COBTMRLIST


#include "coblist.hpp"


template <class ID, int N, class U> class cobtmrlist {
    static_assert(N > 0, "List must have some capacity");

    struct elem {
        ID id;
        U  remaining;
        elem(ID id_)
            : id(id_)
        {
        }
        elem(ID id_, U remain)
            : id(id_)
            , remaining(remain)
        {
        }
        bool operator==(elem const& x) { return id == x.id; }
    };

    coblist<elem, N> timers;

public:
    using index = coblist<elem, N>::link;

    index start(ID id, U duration)
    {
        auto it = timers.lbegin();
        for (; it != timers.lend(); it = timers.fwd(it)) {
            auto tmr = timers.get(it);
            if (tmr.remaining > duration) {
                tmr.remaining -= duration;
                timers.set(it, tmr);
                break;
            }
            duration -= tmr.remaining;
        }
        return timers.insert(it, { id, duration });
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
        if (it != timers.lend()) {
            auto tmr = timers.get(it);
            auto fwd = timers.fwd(it);
            if (fwd != timers.lend()) {
                auto next = timers.get(next);
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
        ID rslt = timers.get(timers.lhead());
        stop(timers.lhead);
        return rslt;
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        auto it = timers.lbegin();
        while (it != timers.lend()) {
            auto tmr = timers.get(it);
            if (tmr.remaining > elapsed) {
                tmr.remaining -= elapsed;
                timers.set(it, tmr);
                break;
            }
            elapsed -= tmr.remaining;
            f(tmr.id);
            auto old = it;
            it       = timers.fwd(it);
            timers.erase(old);
        }
    }

    // std::optional<ID> expired(U proteklo) {}
};

#endif // define      INC_COBTMRLIST
