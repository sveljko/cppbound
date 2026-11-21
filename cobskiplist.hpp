/* Compile time bound skiplists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSKIPLIST)
#define INC_COBSKIPLIST

#include "cobpool.hpp"

#include <bit>
#include <cstdint>
#include <iterator>
#include <utility>    // std::pair


// With `CHECKED` being `true`, the links are checked handles of the pool: the
// link of an erased element is then the link of nothing, also after a new
// element takes its slot.
// With `HIVE` being `true`, which slots hold an element is kept as a bit each
// (see `cobhive`): an eighth of the room for that, a little more to ask.
template <class T, class B, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32, bool CHECKED = false,
          bool HIVE = false>
struct bndskiplist : protected bndpool<T, B, CHECKED, HIVE>
{
    using pool = bndpool<T, B, CHECKED, HIVE>;

    using size_type = typename B::size_type;
    using value_type = T;
    using key_type = T;
    using difference_type = std::ptrdiff_t;
    using key_compare = CMP;
    using value_compare = CMP;
    using pointer = T *;
    using reference = T &;

    using link = typename pool::handle;
    using index = typename B::slot;
    using level = typename B::level;

    static constexpr auto nil = B::nil;

    bndskiplist() { head.fill(nil); }

    using pool::empty;
    using pool::full;
    using pool::live;       // is there an element at the link? O(1).
    using pool::max_size;
    using pool::size;

    class CI
    {
        bndskiplist const *r;
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

        T const &operator*() const
        {
            const node n = r->raw(l);
            index i;
            return (i.be(n.get()) && r->live_node(n)) ? r->at(i) : spot();
        }
        T const *operator->() const { return &**this; }

        bool operator==(CI const &x) const { return (r == x.r) && (l == x.l); }

        friend struct bndskiplist;

    protected:
        CI(bndskiplist const *r_, link l_) : r(r_), l(l_) {}
    };

    using I = CI;
    using iterator = CI;
    using const_iterator = CI;

    CI begin() const { return CI{this, lbegin()}; }
    CI end() const { return CI{this, nil}; }
    CI cbegin() const { return CI{this, lbegin()}; }
    CI cend() const { return CI{this, nil}; }

    // If an equivalent element is already in, returns the iterator to it
    // (and `false`). If there is no room, returns `end()` (and `false`).
    std::pair<I, bool> insert(T const &v)
    {
        const auto where = put(v, v);
        return {I{this, where.first}, where.second};
    }
    std::pair<I, bool> insert(T &&v)
    {
        const auto where = put(v, std::move(v));
        return {I{this, where.first}, where.second};
    }
    template <class... A> std::pair<I, bool> emplace(A &&...a)
    {
        return insert(T(std::forward<A>(a)...));
    }

    // TODO insert(first, last), insert(initializer_list)
    // TODO swap(skiplist), extract(iterator), extract(key), merge(skiplist)

    void clear()
    {
        pool::clear();
        head.fill(nil);
    }

    // Returns the link following the erased one, or `nil` if `pos` is
    // not in the skiplist (in which case nothing is erased).
    link erase(link pos)
    {
        const node gone = this->raw(pos);
        if (!live(pos)) {
            return nil;
        }
        const node rslt = after(gone, level{});
        return take_out(val(gone), gone) ? this->ticket(rslt) : link{nil};
    }
    I erase(CI pos)
    {
        return I{this, (pos.r == this) ? erase(pos.l) : link{nil}};
    }

    size_type erase(T const &v)
    {
        return take_out(v, nil);
    }

    link lbegin() const { return this->ticket(head.get(level{})); }
    link lend() const { return nil; }
    link lfront() const { return lbegin(); }

    link fwd(link l, level lvl = level{}) const
    {
        const node n = this->raw(l);
        return (n == nil) ? link{nil} : this->ticket(after(n, lvl));
    }
    size_type count(T const &v) const
    {
        return is(first_not_before(v), v);
    }
    CI find(T const &v) const
    {
        const node l = first_not_before(v);
        return CI{this, is(l, v) ? this->ticket(l) : link{nil}};
    }
    bool contains(link l) const
    {
        return live(l);
    }
    bool contains(CI pos) const
    {
        return (pos.r == this) && contains(pos.l);
    }
    CI lower_bound(T const &v) const
    {
        return CI{this, lower(v)};
    }
    // The first element `e` with `cmp(v, e)`
    CI upper_bound(T const &v) const
    {
        return CI{this, upper(v)};
    }
    // TODO consider equal_range(key)

    // For debugging
    template <class STREAM>
    void printme(STREAM& out) const
    {
        out << "\nskiplist dump: \n";
        level lvl;
        do {
            out << head.get(lvl).get() << " ";
        } while (lvl.advance());
        out << ".\nnext\n";
        for (auto i : B::slots()) {
            lvl = level{};
            do {
                out << next.get(i, lvl).get() << " ";
            } while (lvl.advance());
            out << ".\n";
        }
        out << "skiplist dump.\n";
    }

protected:
    // What follows is for the views of a skiplist (such as the map).
    // `Q` is anything that `CMP` can compare with an element, both ways.
    // A view may change an element (`with`), if that leaves the order as is.
    using pool::with;

    CI iter_at(link l) const { return CI{this, l}; }

    // Makes an element from `a`, unless there is one equivalent to `key`
    // (which must be how the new element compares), or there is no room.
    // `key` may be (a part of) what `a` refers to.
    template <class Q, class... A> std::pair<link, bool> put(Q const &key, A &&...a)
    {
        const row pred = predecessors(key);
        const node found = after(pred.get(level{}), level{});
        if (is(found, key)) {
            return {this->ticket(found), false};
        }
        const node fresh = this->make_node(std::forward<A>(a)...);
        index ifresh;
        if (!ifresh.be(fresh.get())) {
            return {nil, false};
        }
        level lvl = climb();
        do {
            const node l = pred.get(lvl);
            next.set(ifresh, lvl, after(l, lvl));
            index il;
            if (il.be(l.get())) {
                next.set(il, lvl, fresh);
            }
            else {
                head.set(lvl, fresh);
            }
        } while (lvl.ebb());

        return {this->ticket(fresh), true};
    }

    // The first element that is not ordered before `v`, `nil` if there is none.
    template <class Q> link lower(Q const &v) const
    {
        return this->ticket(first_not_before(v));
    }
    // The first element that `v` is ordered before
    template <class Q> link upper(Q const &v) const
    {
        const node l = first_not_before(v);
        return this->ticket(is(l, v) ? after(l, level{}) : l);
    }

    // Is `l`, which is not ordered before `v`, an element equivalent to `v`?
    template <class Q> bool holds(link l, Q const &v) const
    {
        return is(this->raw(l), v);
    }

    // Take the element equivalent to `v` out, if there is one and, unless
    // `only` is `nil`, if it is the `only` one. Returns the number of
    // elements taken out.
    template <class Q> size_type unlink(Q const &v, link only)
    {
        const node n = this->raw(only);
        return ((only != nil) && (n == nil)) ? 0 : take_out(v, n);
    }

private:
    // Inside, an element is at a node
    using typename pool::node;
    using row = typename B::template perlevel<node>;

    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }

    static bndskiplist& nowhere() { static thread_local bndskiplist none; return none; }

    T const& val(node l) const { index i; i.be(l.get()); return this->at(i); }

    template <class Q> node first_not_before(Q const &v) const
    {
        return after(predecessors(v).get(level{}), level{});
    }
    template <class Q> bool is(node l, Q const &v) const
    {
        return (l != nil) && !CMP{}(v, val(l));
    }

    template <class Q> size_type take_out(Q const &v, node only)
    {
        const row pred = predecessors(v);
        const node found = after(pred.get(level{}), level{});
        index ifound;
        if (!is(found, v) || !ifound.be(found.get())) {
            return 0;
        }
        if ((only != nil) && (only != found)) {
            return 0;
        }
        level lvl = level::greatest();
        do {
            const node l = pred.get(lvl);
            if (after(l, lvl) != found) {
                continue;
            }
            index il;
            if (il.be(l.get())) {
                next.set(il, lvl, next.get(ifound, lvl));
            }
            else {
                head.set(lvl, next.get(ifound, lvl));
            }
        } while (lvl.ebb());
        this->drop_node(found);
        return 1;
    }

    // Each level is half as likely as the one below it
    level climb()
    {
        const auto top = static_cast<unsigned>(level::greatest().get());
        const std::uint_fast32_t mask = (top < 32) ? ((std::uint_fast32_t{1} << top) - 1u) : ~std::uint_fast32_t{0};
        level rslt;
        rslt.be(std::countr_one(static_cast<std::uint_fast32_t>(rng()) & mask & 0xFFFFFFFFu));
        return rslt;
    }

    // The node that follows `l` at level `lvl`, where `nil` stands for
    // "before the first node".
    node after(node l, level lvl) const
    {
        index i;
        return i.be(l.get()) ? next.get(i, lvl) : head.get(lvl);
    }

    // For each level, the last node that is ordered before `v`, or
    // `nil` if there is no such node at that level.
    template <class Q> row predecessors(Q const &v) const
    {
        CMP cmp;
        row pred;
        node l = nil;
        level lvl = level::greatest();
        do {
            node nxt = after(l, lvl);
            while ((nxt != nil) && cmp(val(nxt), v)) {
                l = nxt;
                nxt = after(l, lvl);
            }
            pred.set(lvl, l);
        } while (lvl.ebb());
        return pred;
    }

    row head;
    typename B::template table<node> next;

    RNG rng;
};

template <class T, int N, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using cobskiplist = bndskiplist<T, cobbound<N>, CMP, RNG>;

// The one with checked links
template <class T, int N, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using cobcheckedskiplist = bndskiplist<T, cobbound<N>, CMP, RNG, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class T, int N, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using cobhiveskiplist = bndskiplist<T, cobbound<N>, CMP, RNG, false, true>;
template <class T, int N, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using cobcheckedhiveskiplist = bndskiplist<T, cobbound<N>, CMP, RNG, true, true>;

#endif // define      INC_COBSKIPLIST
