/* Compile time bound timer wheel. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRWHEEL)
#define INC_COBTMRWHEEL


#include "cobtmrlist.hpp"


template <class ID, int N, class U, unsigned SPOKES> class cobtmrwheel {
    static_assert(N > 0, "List must have some capacity");
    static_assert(SPOKES > 1, "Must have more than one spoke");

    using list   = cobtmrlist<ID, N / SPOKES * 2, U>;
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


    template <template <class> class V> constexpr V<ID> stop_first()
    {
        auto it = next;
        do {
            auto& spoke = timers[it.get()];
            if (!spoke.empty()) {
                return spoke.template stop_first<V>();
            }
            if (!it.advance()) {
                it = cobic<0>;
            }
        } while (it != next);

        return {};
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

#endif // define      INC_COBTMRWHEEL
