/* Compile time bound skiplists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSKIPLIST)
#define INC_COBSKIPLIST

#include "cobarray.hpp"

#include "cobhlp.hpp"

#include <random>
#include <utility>    // std::pair


template <class T, int N, class CMP = cobhlp::less<T>>
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

    cobskiplist() : rng(rndev()), leveldist(0, max_level)
    {
        head.fill(nil);
        for (auto i : vacancy.irange())
        {
            vacancy.set(i, i);
        }
        vacant = index::greatest();
    }

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

    class I
    {
        cobskiplist *r;
        link l;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;
        using iterator_category = std::forward_iterator_tag;

        I(I const &x) : r(x.r), l(x.l) {}

        I &operator=(I &x)
        {
            r = x.r;
            l = x.l;
            return *this;
        }
        I &operator=(I &&x)
        {
            r = x.r;
            l = x.l;
            return *this;
        }
        I &operator++()
        {
            l = r->fwd(l);
            return *this;
        }

        T &operator*() { return r->d[l.get()]; }
        T const &operator*() const { return r->d[l.get()]; }
        T *operator->() { return r->d + l.get(); }
        T const *operator->() const { return r->ßd + l.get(); }

        bool operator==(I const &x) const { return (r == x.r) && (l == x.l); }
        bool operator!=(I const &x) const { return (r != x.r) || (l != x.l); }

        friend struct cobskiplist;

    protected:
        I(cobskiplist const *r_, link l_) : r(const_cast<cobskiplist *>(r_)), l(l_)
        {
        }
    };

    class CI
    {
        cobskiplist const *r;
        link l;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;
        using iterator_category = std::forward_iterator_tag;

        CI(CI const &x) : r(x.r), l(x.l) {}
        CI(I const &x) : r(x.r), l(x.l) {}

        CI &operator=(CI &x)
        {
            r = x.r;
            l = x.l;
            return *this;
        }
        CI &operator=(CI &&x)
        {
            r = x.r;
            l = x.l;
            return *this;
        }
        CI &operator++()
        {
            l = r->fwd(l);
            return *this;
        }

        T const &operator*() const { return r->d[l.get()]; }
        T const *operator->() const { return r->d + l.get(); }

        bool operator==(CI const &x) const { return (r == x.r) && (l == x.l); }
        bool operator!=(CI const &x) const { return (r != x.r) || (l != x.l); }

        friend struct cobskiplist;

    protected:
        CI(cobskiplist const *r_, link l_) : r(r_), l(l_) {}
    };

    constexpr I begin() { return I{this, head.get(surface)}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head.get(surface)}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head.get(surface)}; }
    constexpr CI cend() const { return CI{this, nil}; }

    constexpr std::pair<I, bool> insert(T const &v)
    {
        if (!room()) {
            return {I{this, nil}, false};
        }

        auto lkp = lookup(v);
        if (d[lkp.first.get(lkp.second).get()] == v) {
            return {I{this, nil}, false};
        }

        const link fresh = alloc();
        level lvl = climb();
        do {
            link l = lkp.first.get(lvl);
            if (l == nil) {
                head.set(lvl, fresh);
                next[fresh.get()].set(lvl, l);
            }
            else {
                next[fresh.get()].set(lvl, fwd(l, lvl));
                next[l.get()].set(lvl, fresh);
            }
        } while (lvl.ebb());

        d[fresh.get()] = v;

        return {I{this, fresh}, true};
    }
    // TODO insert(first, last), insert(initializer_list)
    // TODO emplace(Args...), swap(skiplist), extract(iterator), extract(key), merge(skiplist)

    constexpr void clear()
    {
        while (!empty()) {
            erase(begin());
        }
    }

    constexpr link erase(link pos)
    {
        if (nil == pos) {
            return nil;
        }
        link rslt = next.get(surface);
        erase(d[pos.get()]);
        return rslt;
    }
    constexpr I erase(CI pos)
    {
        return I{this, erase(pos.l)};
    }

    constexpr unsigned erase(T const &v)
    {
        unsigned rslt = 0;
        level lvl = level::greatest();
        do {
            const link l = head.get(lvl);
            if (nil == l) {
                continue;
            }
            const link sqnt = fwd(l, lvl);
            if (v == d[l.get()]) {
                head.set(lvl, sqnt);
                next[l.get()].set(lvl, nil);
                ++rslt;
            }
            else {
                rslt += erasebody(lvl, l, sqnt, v);
            }
        } while (lvl.ebb());
        return rslt > 0;
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
        auto pos = lookup(v);
        return pos.first.get(pos.second) != nil;
    }
    constexpr I find(T const &v)
    {
        auto lkp = lookup(v);
        return (d[lkp.first.get(lkp.second).get()] == v) ? I{this, lkp} : I{this, nil};
    }
    constexpr CI find(T const &v) const
    {
        auto lkp = lookup(v);
        return (d[lkp.first.get(lkp.second).get()] == v) ? CI{this, lkp} : CI{this, nil};
    }
    constexpr bool contains(link l) const
    {
        return count(d[l.get()]) > 0;
    }
    constexpr bool contains(CI pos) const
    {
        return (pos.r == this) && contains(pos.l);
    }
    constexpr I lower_bound(T const &v)
    {
        auto const lkp = lookup(v);
        return I{this, lkp.first.get(lkp.second)};
    }
    constexpr CI lower_bound(T const &v) const
    {
        auto const lkp = lookup(v);
        return CI{this, lkp.first.get(lkp.second)};
    }
    // TODO upper_bound(key)
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
    bool room() const
    {
        return vacant != nil;
    }
    link alloc()
    {
        index vi;
        if (vi.be(vacant.get())) {
            link rslt = vacancy.get(vi);
            if (!vacant.ebb()) {
                vacant = nil;
            }
            return rslt;
        }
        return nil;
    }
    void dealloc(link l)
    {
        if (room()) {
            vacancy.set(vacant, l);
            vacant.advance();
        }
    }

    level climb()
    {
        level rslt;
        rslt.be(log2(leveldist(rng)));
        return rslt;
    }

    constexpr std::pair<node, level> lookup(T const &v, level lvl = cobic<max_level>) const
    {
        node pos;
        pos.fill(nil);
        link l = head.get(lvl);
        while ((nil == l) && lvl.ebb()) {
            l = head.get(lvl);
        }

        CMP cmp;
        while (l != nil)
        {
            const link nxt = fwd(l, lvl);
            if ((nxt != nil) && !cmp(v, d[nxt.get()])) {
                l = nxt;
            }
            else {
                pos.set(lvl, l);
                if (!lvl.ebb()) {
                    return {pos, lvl};
                }
            }
        }
        return {pos, lvl};
    }

    constexpr unsigned erasebody(level lvl, link l, link sqnt, T const &v)
    {
        CMP cmp;
        unsigned rslt = 0;
        while (sqnt != nil) {
            if (v == d[sqnt.get()]) {
                next[l.get()].set(lvl, fwd(sqnt,lvl));
                next[sqnt.get()].set(lvl, nil);
                rslt = 1;
                break;
            }
            else if (cmp(v, d[sqnt.get()])) {
                break;
            }
            l = sqnt;
            sqnt = fwd(sqnt, lvl);
        }
        return rslt;
    }

    T d[N + 1];
    node head;
    node next[N];
    link vacant;
    cobarray<link, N> vacancy;

    // TODO parameterize RNG
    std::random_device rndev;
    std::mt19937 rng;
    std::uniform_int_distribution<std::mt19937::result_type> leveldist;
};

#endif // define      INC_COBSKIPLIST
