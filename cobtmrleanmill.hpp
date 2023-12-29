/* Compile time bound timer lean mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRMILL)
#define INC_COBTMRMILL


#include "cobarray.hpp"
#include "cobmatrix.hpp"


template <class ID, int N, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
class cobtmrleanmill {
    static_assert(N > 0, "Mill must have some timers");
    static_assert(LVL * DIM > 0, "Mill must have some spokes");
    static_assert(LVL * DIM < N, "Too many spokes");

    static constexpr auto spokedim = N / (LVL * DIM) + 1;

    using level     = cobint<0, LVL - 1>;
    using lvlindex  = cobint<0, DIM - 1>;
    using spoke     = cobarray<ID, spokedim>;
    using spokespos = cobint<0, spokedim>;

    spoke                         timers[LVL][DIM];
    cobmatrix<unsigned, LVL, DIM> active;
    cobarray<lvlindex, LVL>       next;

public:
    cobtmrleanmill()
    {
        active.fill(0);
        next.fill(cobic<0>);
    }

    static constexpr U max_duration{ 1 << (DIM * cobhlp::log2(LVL)) };

    struct index {
        level    lvl;
        lvlindex ispoke;
        spokepos idx;

        constexpr bool valid(index i) const { return i.idx != idx.greatest(); }
    };

    constexpr index spoke_from_duration(U duration)
    {
        level    lvl;
        unsigned d = duration;
        do {
            lvlindex idx;
            if (idx.be(d)) {
                return { lvl, idx, cobic<0> };
            }
            d = d / DIM;
        } while (lvl.advance());
        return { lvl, cobic<0>, spokepos::greatest() };
    }

    constexpr index start(ID id, U duration)
    {
        index i = index_from_duration(U);
        if (spoke::nil != i.idx) {
            auto& spoke = timers[i.lvl.get()][i.ispoke.get()];
            auto  pos   = active.get({ i.lvl, i.ispoke });
            if (spoke.maybe_set(pos, id)) {
                active.set({ i.lvl, i.ispoke }, pos + 1);
            }
        }
        return i;
    }

    int stop(ID id)
    {
        level lvl;
        do {
            spoke spoke;
            do {
	      for (unsigned i = 0; i < active.get({lvl, spoke}); ++i) {
                    NULIFY nlf;
                    auto* p = timers[it.lvl.get()][it.ispoke.get()].begin() + i;
                    if (*p == id) {

                        nlf(*p);
                        return 0;
                    }
                }
            } while (spoke.advance());
        } while (lvl.advance());
        return -1;
    }

    void stop(index it)
    {
        if (it.valid()) {
            NULIFY nlf;
            auto*  p = timers[it.lvl.get()][it.ispoke.get()].begin() + it.idx;
            nlf(*p);
        }
    }

    template <template <class> class V> constexpr V<ID> stop_first()
    {
        level lvl;
        do {
            auto ispoke = next.get(lvl);
            do {
                auto& list = timers[lvl.get()][ispoke.get()];
                if (active.get({lvl, ispoke}) > 0) {
                    NULIFY nlf;
                    auto*  p    = timers[lvl.get()][ispoke.get()].begin();
                    ID     rslt = *p;
                    nlf(*p);
                    return rslt;
                }
                if (!ispoke.advance()) {
                    it            = cobic<0>;
                    auto next_lvl = lvl;
                    if (next_lvl.advance()) {
                        auto nxt_lvl_idx = next.get(next_lvl);
                        auto& nxtlist = timers[next_lvl.get()][nxt_lvl_idx.get()];
                        if (active.get({next_lvl, nxt_lvl_idx)) > 0) {
                            NULIFY nlf;
                            auto*  p    = nxtlist.begin();
                            ID     rslt = *p;
                            nlf(*p);
                            return rslt;
                        }
                    }
                }
            } while (ispoke != next.get());
        } while (lvl.advance());
        return {};
    }

    template <class F> void process_expired(U elapsed, F f)
    {
        for (U i = 0; i < elapsed; ++i) {
            level lvl;
            do {
                auto ispoke = next.get(lvl);
                for (auto cons& id : timers[lvl.get()][ispoke.get()]) {
                    f(id);
                }
                active.set({lvl, ispoke.get}, 0);

                if (ispoke.advance()) {
                    break;
                }
                next.set(lvl, cobic<0>);
            } while (lvl.advance());
        }
    }

    // std::optional<ID> expired(U proteklo) {}
};

#endif // define      INC_COBTMRWHEEL
