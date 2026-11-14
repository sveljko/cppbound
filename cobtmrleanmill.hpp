/* Compile time bound timer lean mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTMRLEANMILL)
#define INC_COBTMRLEANMILL


#include "cobarray.hpp"
#include "cobhlp.hpp"
#include "cobmatrix.hpp"


template <class ID, int N, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
class cobtmrleanmill {
    static_assert(N > 0, "Mill must have some timers");
    static_assert(LVL * DIM > 0, "Mill must have some spokes");
    static_assert(LVL * DIM < N, "Too many spokes");

    static constexpr auto spokedim = N / (LVL * DIM) + 1;

    using level    = cobint<0, LVL - 1>;
    using lvlindex = cobint<0, DIM - 1>;
    using spoke    = cobarray<ID, spokedim>;
    using spokepos = cobint<0, spokedim>;

    spoke                         timers[LVL][DIM];
    cobmatrix<unsigned, LVL, DIM> active;
    cobarray<lvlindex, LVL>       next;

public:
    using tmrID = ID;

    cobtmrleanmill()
    {
        active.fill(0);
        next.fill(cobic<0>);
    }

    static constexpr U max_duration{ cobhlp::pow(DIM, LVL) - 1 };

    struct index {
        level    lvl;
        lvlindex ispoke;
        spokepos idx;

        constexpr bool valid() const { return idx != idx.greatest(); }
    };

    constexpr index spoke_from_duration(U duration)
    {
        level lvl;
        // Before anything else, as the duration might not fit our integer
        if ((duration < U{ 0 }) || (duration > max_duration)) {
            return { lvl, cobic<0>, spokepos::greatest() };
        }
        unsigned d = static_cast<unsigned>(duration.count());
        do {
            lvlindex idx;
            if (idx.be(d)) {
                d = (d + next.get(lvl).get()) % (idx.greatest().get() + 1);
                idx.be(d);
                return { lvl, idx, cobic<0> };
            }
            d = d / DIM;
        } while (lvl.advance());
        return { lvl, cobic<0>, spokepos::greatest() };
    }

    constexpr index start(ID id, U duration)
    {
        index i = spoke_from_duration(duration);
        if (i.valid()) {
            auto& spoke = timers[i.lvl.get()][i.ispoke.get()];
            auto  pos   = active.get({ i.lvl, i.ispoke });
            if (i.idx.be(pos) && spoke.maybe_set(pos, id)) {
                active.set({ i.lvl, i.ispoke }, pos + 1);
            }
            else {
                i.idx = spokepos::greatest();
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
        if (it.valid() && (static_cast<unsigned>(it.idx.get()) < active.get({ it.lvl, it.ispoke }))) {
            NULIFY nlf;
            auto p = timers[it.lvl.get()][it.ispoke.get()].begin() + it.idx.get();
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

#endif // define      INC_COBTMRLEANMILL
