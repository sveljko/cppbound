/* Compile time bound skiplists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSKIPLIST)
#define INC_COBSKIPLIST

#include "cobarray.hpp"
#include "cobstore.hpp"

#include "cobhlp.hpp"

#include <bit>
#include <cstdint>
#include <utility>    // std::pair


template <class T, int N, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
struct cobskiplist
{
    static_assert(cobhlp::log2(N) > 2, "Skiplist too small");
    static constexpr auto max_level = cobhlp::log2(N);

    using size_type = unsigned;
    using value_type = T;
    using key_type = T;
    using difference_type = std::ptrdiff_t;
    using key_compare = CMP;
    using value_compare = CMP;
    using pointer = T *;
    using reference = T &;

    using link = cobint<0, N>;
    using index = cobint<0, N - 1>;
    using node = cobarray<link, max_level + 1>;
    using level = cobint<0, max_level>;

    static constexpr auto nil = cobic<N>;
    static constexpr auto surface = cobic<0>;

    cobskiplist()
    {
        head.fill(nil);
        used.fill(false);
        for (auto i : vacancy.irange())
        {
            vacancy.set(i, i);
        }
        vacant = nil;
    }

    cobskiplist(cobskiplist const&) = delete;
    cobskiplist& operator=(cobskiplist const&) = delete;
    ~cobskiplist() { clear(); }

    constexpr bool empty() const noexcept { return head.get(surface) == nil; }

    constexpr unsigned size() const noexcept
    {
        link l = head.get(surface);
        unsigned c = 0;
        while (l != nil) {
            ++c;
            l = fwd(l);
        }
        return c;
    }
    constexpr unsigned max_size() const noexcept { return N; }

    class CI
    {
        cobskiplist const *r;
        link l;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T const*;
        using reference = T const&;
        using iterator_category = std::forward_iterator_tag;

        CI() : r(&nowhere()), l(nil) {}
        CI operator++(int) { CI t = *this; ++*this; return t; }

        CI &operator++()
        {
            l = r->fwd(l);
            return *this;
        }

        T const &operator*() const { index i; return (i.be(l.get()) && r->live(l)) ? r->store.at(i) : spot(); }
        T const *operator->() const { return &**this; }

        bool operator==(CI const &x) const { return (r == x.r) && (l == x.l); }

        friend struct cobskiplist;

    protected:
        CI(cobskiplist const *r_, link l_) : r(r_), l(l_) {}
    };

    using I = CI;
    using iterator = CI;
    using const_iterator = CI;

    constexpr I begin() { return I{this, head.get(surface)}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head.get(surface)}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head.get(surface)}; }
    constexpr CI cend() const { return CI{this, nil}; }

    // If an equivalent element is already in, returns the iterator to it
    // (and `false`). If there is no room, returns `end()` (and `false`).
    constexpr std::pair<I, bool> insert(T const &v) { return put(v, v); }
    constexpr std::pair<I, bool> insert(T &&v) { return put(v, std::move(v)); }
    template <class... A> constexpr std::pair<I, bool> emplace(A &&...a)
    {
        return insert(T(std::forward<A>(a)...));
    }

    // Is there an element at `l`? O(1).
    constexpr bool live(link l) const
    {
        index i;
        return i.be(l.get()) && used.get(i);
    }

private:
    // `key` is the key of the element to be made from `v`; they may be the same object
    template <class U> constexpr std::pair<I, bool> put(T const &key, U &&v)
    {
        T const &v_ = key;
        const node pred = predecessors(v_);
        const link found = after(pred.get(surface), surface);
        if (holds(found, v_)) {
            return {I{this, found}, false};
        }
        if (!room()) {
            return {I{this, nil}, false};
        }

        const link fresh = alloc();
        index ifresh;
        if (!ifresh.be(fresh.get())) {
            return {I{this, nil}, false};
        }
        level lvl = climb();
        do {
            const link l = pred.get(lvl);
            next[ifresh.get()].set(lvl, after(l, lvl));
            index il;
            if (il.be(l.get())) {
                next[il.get()].set(lvl, fresh);
            }
            else {
                head.set(lvl, fresh);
            }
        } while (lvl.ebb());

        used.set(ifresh, true);
        store.make(ifresh, std::forward<U>(v));

        return {I{this, fresh}, true};
    }

public:
    // TODO insert(first, last), insert(initializer_list)
    // TODO emplace(Args...), swap(skiplist), extract(iterator), extract(key), merge(skiplist)

    constexpr void clear()
    {
        while (!empty()) {
            erase(lbegin());
        }
    }

    // Returns the link following the erased one, or `nil` if `pos` is
    // not in the skiplist (in which case nothing is erased).
    constexpr link erase(link pos)
    {
        if (!live(pos)) {
            return nil;
        }
        const link rslt = fwd(pos);
        return unlink(val(pos), pos) ? rslt : link{nil};
    }
    constexpr I erase(CI pos)
    {
        return I{this, (pos.r == this) ? erase(pos.l) : link{nil}};
    }

    constexpr unsigned erase(T const &v)
    {
        return unlink(v, nil);
    }

    constexpr link lbegin() const { return head.get(surface); }
    constexpr link lend() const { return nil; }
    constexpr link lfront() const { return head.get(surface); }

    constexpr link fwd(link l, level lvl=surface) const
    {
        index lidx;
        if (lidx.be(l.get())) {
            return next[lidx.get()].get(lvl);
        }
        else {
            return nil;
        }
    }
    constexpr size_type count(T const &v) const
    {
        return holds(lower(v), v);
    }
    constexpr I find(T const &v)
    {
        const link l = lower(v);
        return I{this, holds(l, v) ? l : link{nil}};
    }
    constexpr CI find(T const &v) const
    {
        const link l = lower(v);
        return CI{this, holds(l, v) ? l : link{nil}};
    }
    constexpr bool contains(link l) const
    {
        return live(l);
    }
    constexpr bool contains(CI pos) const
    {
        return (pos.r == this) && contains(pos.l);
    }
    constexpr I lower_bound(T const &v)
    {
        return I{this, lower(v)};
    }
    constexpr CI lower_bound(T const &v) const
    {
        return CI{this, lower(v)};
    }
    // The first element `e` with `cmp(v, e)`
    constexpr I upper_bound(T const &v)
    {
        return I{this, upper(v)};
    }
    constexpr CI upper_bound(T const &v) const
    {
        return CI{this, upper(v)};
    }
    // TODO consider equal_range(key)

    // For debugging
    template <class STREAM>
    void printme(STREAM& out)
    {
        out << "\nskiplist dump: \n";
        print(out, head);
        out << "next\n";
        for (auto &x : next) {
            print(out, x);
        }
        out << "skiplist dump.\n";
    }

private:
    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }
    constexpr T const& val(link l) const { index i; i.be(l.get()); return store.at(i); }

    static cobskiplist& nowhere() { static thread_local cobskiplist none; return none; }

    // `vacancy` is a stack of free nodes, `vacant` is the number of them
    bool room() const
    {
        return vacant != cobic<0>;
    }
    link alloc()
    {
        index vi;
        if (vacant.ebb() && vi.be(vacant.get())) {
            return vacancy.get(vi);
        }
        return nil;
    }
    void dealloc(link l)
    {
        index vi;
        if (vi.be(vacant.get())) {
            vacancy.set(vi, l);
            vacant.advance();
        }
    }

    // Each level is half as likely as the one below it
    level climb()
    {
        level rslt;
        rslt.be(std::countr_one(static_cast<std::uint_fast32_t>(rng()) & ((1u << max_level) - 1u)));
        return rslt;
    }

    // The node that follows `l` at level `lvl`, where `nil` stands for
    // "before the first node".
    constexpr link after(link l, level lvl) const
    {
        return (l == nil) ? head.get(lvl) : fwd(l, lvl);
    }

    // For each level, the last node that is ordered before `v`, or
    // `nil` if there is no such node at that level.
    constexpr node predecessors(T const &v) const
    {
        CMP cmp;
        node pred;
        link l = nil;
        level lvl = level::greatest();
        do {
            link nxt = after(l, lvl);
            while ((nxt != nil) && cmp(val(nxt), v)) {
                l = nxt;
                nxt = fwd(l, lvl);
            }
            pred.set(lvl, l);
        } while (lvl.ebb());
        return pred;
    }

    // The first node that is not ordered before `v`, `nil` if there is none.
    constexpr link lower(T const &v) const
    {
        return after(predecessors(v).get(surface), surface);
    }

    constexpr link upper(T const &v) const
    {
        const link l = lower(v);
        return holds(l, v) ? fwd(l) : l;
    }

    // Is `l`, which is not ordered before `v`, a node equivalent to `v`?
    constexpr bool holds(link l, T const &v) const
    {
        return (l != nil) && !CMP{}(v, val(l));
    }

    // Take the node equivalent to `v` out, if there is one and, unless
    // `only` is `nil`, if it is the `only` node. Returns the number of
    // nodes taken out.
    constexpr unsigned unlink(T const &v, link only)
    {
        const node pred = predecessors(v);
        const link found = after(pred.get(surface), surface);
        index ifound;
        if (!holds(found, v) || !ifound.be(found.get())) {
            return 0;
        }
        if ((only != nil) && (only != found)) {
            return 0;
        }
        level lvl = level::greatest();
        do {
            const link l = pred.get(lvl);
            if (after(l, lvl) != found) {
                continue;
            }
            index il;
            if (il.be(l.get())) {
                next[il.get()].set(lvl, fwd(found, lvl));
            }
            else {
                head.set(lvl, fwd(found, lvl));
            }
        } while (lvl.ebb());
        used.set(ifound, false);
        store.drop(ifound);
        dealloc(found);
        return 1;
    }

    cobstore<T, N> store;
    cobarray<bool, N> used;
    node head;
    node next[N];
    link vacant;
    cobarray<link, N> vacancy;

    RNG rng;
};

#endif // define      INC_COBSKIPLIST
