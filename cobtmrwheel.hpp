/* Compile time bound timer wheel. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRWHEEL)
#define INC_COBTMRWHEEL


#include "cobtmrlist.hpp"


// `L` is the bound of the number of timers of a spoke.
// With `CHECKED` being `true`, the index of a timer is checked: once that
// timer is not running, it is the index of nothing.
// With `HIVE` being `true`, which slots hold a timer is kept as a bit each.
template <class ID, class L, class U, unsigned SPOKES, bool CHECKED = false, bool HIVE = false>
class bndtmrwheel {
    static_assert(SPOKES > 1, "Must have more than one spoke");

    using list   = bndtmrlist<ID, L, U, CHECKED, HIVE>;
    using ispoke = cobint<0, SPOKES - 1>;
    list   timers[SPOKES];
    ispoke next;

public:
    using tmrID = ID;

    struct index {
        ispoke     spoke;
        typename list::index idx;
    };

    index start(ID id, U duration)
    {
        // Soonest to expire is on the next tick, which looks at spoke `next`
        auto ticks = duration.count();
        if (ticks < 1) {
            ticks = 1;
        }
        ispoke spoke;
        spoke.be(static_cast<int>((next.get() + (ticks - 1) % SPOKES) % SPOKES));
        return { spoke, timers[spoke.get()].start(id, U{ ticks }) };
    }

    int stop(ID id)
    {
        for (auto& l : timers) {
            if (l.stop(id) == 0) {
                return 0;
            }
        }
        return -1;
    }

    int stop(index it) { return timers[it.spoke.get()].stop(it.idx); }


    // The first timer of a spoke expires when the spoke is looked at in the
    // turn its time runs out in, so the spoke that is looked at sooner need
    // not have the timer that expires sooner: all the spokes are compared.
    template <template <class> class V> constexpr V<ID> stop_first()
    {
        using ticks_t = unsigned long long;
        bool    any   = false;
        ispoke  first = next;
        ticks_t least = 0;
        ticks_t ahead = 1;      // the spoke `next` is looked at on the next tick
        auto    it    = next;
        do {
            auto& spoke = timers[it.get()];
            if (!spoke.empty()) {
                const auto    left  = spoke.first_remaining().count();
                const ticks_t turns = (left > 0) ? (static_cast<ticks_t>(left) - 1) / SPOKES : 0;
                const ticks_t when  = ahead + turns * SPOKES;
                if (!any || (when < least)) {
                    any   = true;
                    first = it;
                    least = when;
                }
            }
            ++ahead;
            if (!it.advance()) {
                it = cobic<0>;
            }
        } while (it != next);

        if (!any) {
            return {};
        }
        return timers[first.get()].template stop_first<V>();
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        for (auto ticks = elapsed.count(); ticks > 0; --ticks) {
            // First turn the wheel, so that `f` can start timers
            const ispoke spoke = next;
            if (!next.advance()) {
                next = cobic<0>;
            }
            timers[spoke.get()].process_expired(U{ SPOKES }, f);
        }
    }

    // std::optional<ID> expired(U proteklo) {}
};

// Each spoke has room for twice its share of the N timers
template <class ID, int N, class U, unsigned SPOKES>
using cobtmrwheel = bndtmrwheel<ID, cobbound<N / SPOKES * 2>, U, SPOKES>;

// The one with checked indices
template <class ID, int N, class U, unsigned SPOKES>
using cobcheckedtmrwheel = bndtmrwheel<ID, cobbound<N / SPOKES * 2>, U, SPOKES, true>;

// The ones where which slots hold a timer is kept as a bit each (see `cobhive`)
template <class ID, int N, class U, unsigned SPOKES>
using cobhivetmrwheel = bndtmrwheel<ID, cobbound<N / SPOKES * 2>, U, SPOKES, false, true>;
template <class ID, int N, class U, unsigned SPOKES>
using cobcheckedhivetmrwheel = bndtmrwheel<ID, cobbound<N / SPOKES * 2>, U, SPOKES, true, true>;

#endif // define      INC_COBTMRWHEEL
