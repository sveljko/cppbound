/* Compile time bound timer mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRMILL)
#define INC_COBTMRMILL


#include "coblist.hpp"
#include "cobhlp.hpp"


template <class ID, int N, class U, unsigned LVL = 5, unsigned DIM = 64>
class cobtmrmill {
    static_assert(N > 0, "Mill must have some timers");
    static_assert(LVL * DIM > 0, "Mill must have some spokes");
    static_assert(LVL * DIM < N, "Too many spokes");

    static constexpr auto spokedim = N / (LVL * DIM) + 1;

    using level    = cobint<0, LVL - 1>;
    using lvlindex = cobint<0, DIM - 1>;
    using spoke    = coblist<ID, spokedim>;

    spoke                   timers[LVL][DIM];
    cobarray<lvlindex, LVL> next;

public:
    using tmrID = ID;

    cobtmrmill() { next.fill(cobic<0>); }

    static constexpr U max_duration{ 1 << (DIM * cobhlp::log2(LVL)) };

    struct index {
        level                lvl;
        lvlindex             ispoke;
        typename spoke::link idx;

        constexpr bool valid(index i) const { return i.idx != spoke::nil; }
    };

    constexpr index spoke_from_duration(U duration)
    {
        level    lvl;
        unsigned d = duration.count();
        do {
            lvlindex idx;
            if (idx.be(d)) {
                d = (d + next.get(lvl).get()) % (idx.greatest().get() + 1);
                idx.be(d);
                return { lvl, idx, cobic<0> };
            }
            d = d / DIM;
        } while (lvl.advance());
        return { lvl, cobic<0>, spoke::nil };
    }

    constexpr index start(ID id, U duration)
    {
        index i = spoke_from_duration(duration);
        if (spoke::nil != i.idx) {
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
                for (auto const& id : timers[lvl.get()][ispoke.get()]) {
                    f(id);
                }
                timers[lvl.get()][ispoke.get()].clear();

                if (ispoke.advance()) {
                    next.set(lvl, ispoke);
                    break;
                }
                next.set(lvl, cobic<0>);
            } while (lvl.advance());
        }
    }

    // std::optional<ID> expired(U proteklo) {}
};

#endif // define      INC_COBTMRWHEEL
