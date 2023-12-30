/* Compile time bound timer wheel. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRWHEEL)
#define INC_COBTMRWHEEL


#include "coblist.hpp"


template <class ID, int N, class U, unsigned SPOKES> class cobtmrwheel {
    static_assert(N > 0, "List must have some capacity");
    static_assert(SPOKES > 1, "Must have more than one spoke");

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
        unsigned i = duration.count() % SPOKES;
        ispoke   spoke;
        spoke.be(i);
        return { spoke, timers[spoke.get()].start(id, duration) };
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
            auto  rslt  = spoke.template stop_first<V>();
            if (rslt) {
                return rslt;
            }
            if (!it.advance()) {
                it = cobic<0>;
            }
        } while (it != next);

        return {};
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        for (unsigned i = 0; i < elapsed.count(); ++i) {
            timers[next.get()].process_expired(U{ SPOKES }, f);
            if (!next.advance()) {
                next = cobic<0>;
            }
        }
    }

    // std::optional<ID> expired(U proteklo) {}
};

#endif // define      INC_COBTMRWHEEL
