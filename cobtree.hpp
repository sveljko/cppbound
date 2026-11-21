/* Compile time bound balanced search tree. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTREE)
#define      INC_COBTREE


#include "cobpool.hpp"

#include <iterator>
#include <utility>    // std::pair


// An AVL tree: the heights of the two subtrees of a node differ by at most
// one, so a tree of n elements is no higher than about 1.44 * log2(n).
// Thus finding, inserting and erasing an element take O(log n) steps in the
// worst case, not just on average - which is what a skiplist or a hash
// table give. Of the balanced trees, this one has the least height, so the
// tightest bound, and that is why it is the one here.
// Nothing is recursive, so the stack use does not depend on the size.
//
// The interface is that of the skiplist: a set of unique elements, ordered
// by `CMP`. The elements stay where they are for as long as they are in:
// a link (and an iterator) of an element is good until that one is erased.
//
// With `CHECKED` being `true`, the links are checked handles of the pool: the
// link of an erased element is then the link of nothing, also after a new
// element takes its slot.
// With `HIVE` being `true`, which slots hold an element is kept as a bit each
// (see `cobhive`): an eighth of the room for that, a little more to ask.
template <class T, class B, class CMP = cobhlp::less<T>, bool CHECKED = false, bool HIVE = false>
struct bndtree : protected bndpool<T, B, CHECKED, HIVE> {
    using pool = bndpool<T, B, CHECKED, HIVE>;

    using size_type       = typename B::size_type;
    using value_type      = T;
    using key_type        = T;
    using difference_type = std::ptrdiff_t;
    using key_compare     = CMP;
    using value_compare   = CMP;
    using pointer         = T*;
    using reference       = T&;

    using link  = typename pool::handle;
    using index = typename B::slot;

    static constexpr auto nil = B::nil;

    bndtree() : root(nil) { lean.fill(0); }

    using pool::empty;
    using pool::full;
    using pool::live;       // is there an element at the link? O(1).
    using pool::max_size;
    using pool::size;

    class CI {
        bndtree const* r;
        link           l;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T const*;
        using reference         = T const&;
        using iterator_category = std::bidirectional_iterator_tag;

        CI() : r(&nowhere()), l(nil) {}

        CI& operator++()
        {
            l = r->fwd(l);
            return *this;
        }
        CI  operator++(int) { CI t = *this; ++*this; return t; }
        CI& operator--()
        {
            l = (l == nil) ? r->lback() : r->bck(l);
            return *this;
        }
        CI operator--(int) { CI t = *this; --*this; return t; }

        T const& operator*() const
        {
            const node n = r->raw(l);
            index      i;
            return (i.be(n.get()) && r->live_node(n)) ? r->at(i) : spot();
        }
        T const* operator->() const { return &**this; }

        bool operator==(CI const& x) const { return (r == x.r) && (l == x.l); }

        friend struct bndtree;

    protected:
        CI(bndtree const* r_, link l_) : r(r_), l(l_) {}
    };
    using I              = CI;
    using iterator       = CI;
    using const_iterator = CI;

    CI begin() const { return CI{ this, lbegin() }; }
    CI end() const { return CI{ this, nil }; }
    CI cbegin() const { return CI{ this, lbegin() }; }
    CI cend() const { return CI{ this, nil }; }

    // If an equivalent element is already in, returns the iterator to it
    // (and `false`). If there is no room, returns `end()` (and `false`).
    std::pair<I, bool> insert(T const& v)
    {
        const auto where = put(v, v);
        return { I{ this, where.first }, where.second };
    }
    std::pair<I, bool> insert(T&& v)
    {
        const auto where = put(v, std::move(v));
        return { I{ this, where.first }, where.second };
    }
    template <class... A> std::pair<I, bool> emplace(A&&... a) { return insert(T(std::forward<A>(a)...)); }

    void clear()
    {
        pool::clear();
        root = nil;
    }

    // Returns the link following the erased one, or `nil` if `pos` is
    // not in the tree (in which case nothing is erased).
    link erase(link pos)
    {
        const node gone = this->raw(pos);
        if (!live(pos)) {
            return nil;
        }
        const node rslt = beside(gone, true);
        take_out(gone);
        return this->ticket(rslt);
    }
    I erase(CI pos) { return I{ this, (pos.r == this) ? erase(pos.l) : link{ nil } }; }
    size_type erase(T const& v) { return unlink(v, nil); }

    // The first and the last element: O(log n)
    link lbegin() const { return this->ticket(furthest(root, false)); }
    link lend() const { return nil; }
    link lfront() const { return lbegin(); }
    link lback() const { return this->ticket(furthest(root, true)); }

    // The next and the previous element: O(log n), and O(1) on average
    link fwd(link l) const { return this->ticket(beside(this->raw(l), true)); }
    link bck(link l) const { return this->ticket(beside(this->raw(l), false)); }

    size_type count(T const& v) const { return is(first_not_before(v), v); }
    CI        find(T const& v) const
    {
        const node l = first_not_before(v);
        return CI{ this, is(l, v) ? this->ticket(l) : link{ nil } };
    }
    bool contains(link l) const { return live(l); }
    bool contains(CI pos) const { return (pos.r == this) && contains(pos.l); }
    // The first element that is not ordered before `v`
    CI lower_bound(T const& v) const { return CI{ this, lower(v) }; }
    // The first element that `v` is ordered before
    CI upper_bound(T const& v) const { return CI{ this, upper(v) }; }

    // The number of elements on the longest way from the root down. O(log n).
    unsigned height() const
    {
        unsigned h = 0;
        for (node l = root; l != nil; l = kid(l, leaning(l) > 0)) {
            ++h;
        }
        return h;
    }
    // For testing: is all of it as it should be? Looks at everything.
    bool sound() const
    {
        size_type n      = 0;
        node      before = nil;
        for (node l = furthest(root, false); l != nil; l = beside(l, true)) {
            if (!this->live_node(l) || ((before != nil) && !CMP{}(val(before), val(l))) || (++n > size())) {
                return false;
            }
            for (bool right : { false, true }) {
                const node k = kid(l, right);
                if ((k != nil) && (parent(k) != l)) {
                    return false;
                }
            }
            const int diff = static_cast<int>(tall(kid(l, true))) - static_cast<int>(tall(kid(l, false)));
            if ((diff != leaning(l)) || (diff < -1) || (diff > 1)) {
                return false;
            }
            before = l;
        }
        return (n == size()) && ((root == nil) || (parent(root) == nil));
    }

protected:
    // What follows is for the views of a tree (such as the map).
    // `Q` is anything that `CMP` can compare with an element, both ways.
    // A view may change an element (`with`), if that leaves the order as is.
    using pool::with;

    CI iter_at(link l) const { return CI{ this, l }; }

    // Makes an element from `a`, unless there is one equivalent to `key`
    // (which must be how the new element compares), or there is no room.
    // `key` may be (a part of) what `a` refers to.
    template <class Q, class... A> std::pair<link, bool> put(Q const& key, A&&... a)
    {
        CMP  cmp;
        node above = nil;
        bool right = false;
        for (node l = root; l != nil; l = kid(l, right)) {
            if (cmp(key, val(l))) {
                right = false;
            }
            else if (cmp(val(l), key)) {
                right = true;
            }
            else {
                return { this->ticket(l), false };
            }
            above = l;
        }
        const node fresh = this->make_node(std::forward<A>(a)...);
        if (fresh == nil) {
            return { nil, false };
        }
        set_kid(fresh, false, nil);
        set_kid(fresh, true, nil);
        set_leaning(fresh, 0);
        hang(fresh, above, right);

        // Up from the new leaf: the side it is on has become higher
        for (node z = fresh, x = above; x != nil; x = parent(z)) {
            const bool on_right = (z == kid(x, true));
            const int  toward   = on_right ? 1 : -1;
            if (leaning(x) == toward) {
                const node g       = parent(x);
                const bool g_right = (g != nil) && (x == kid(g, true));
                hang((leaning(z) == -toward) ? rotate_twice(x, z, on_right) : rotate(x, z, on_right), g, g_right);
                break;
            }
            if (leaning(x) == -toward) {
                set_leaning(x, 0);
                break;
            }
            set_leaning(x, toward);
            z = x;
        }
        return { this->ticket(fresh), true };
    }

    // The first element that is not ordered before `v`, `nil` if there is none.
    template <class Q> link lower(Q const& v) const { return this->ticket(first_not_before(v)); }
    // The first element that `v` is ordered before
    template <class Q> link upper(Q const& v) const
    {
        const node l = first_not_before(v);
        return this->ticket(is(l, v) ? beside(l, true) : l);
    }
    // Is `l`, which is not ordered before `v`, an element equivalent to `v`?
    template <class Q> bool holds(link l, Q const& v) const { return is(this->raw(l), v); }

    // Take the element equivalent to `v` out, if there is one and, unless
    // `only` is `nil`, if it is the `only` one. Returns the number of
    // elements taken out.
    template <class Q> size_type unlink(Q const& v, link only)
    {
        const node found = first_not_before(v);
        if (!is(found, v) || ((only != nil) && (this->raw(only) != found))) {
            return 0;
        }
        take_out(found);
        return 1;
    }

private:
    // Inside, an element is at a node
    using typename pool::node;

    T const& val(node l) const { index i; i.be(l.get()); return this->at(i); }

    template <class Q> node first_not_before(Q const& v) const
    {
        CMP  cmp;
        node rslt = nil;
        for (node l = root; l != nil;) {
            if (cmp(val(l), v)) {
                l = kid(l, true);
            }
            else {
                rslt = l;
                l    = kid(l, false);
            }
        }
        return rslt;
    }
    template <class Q> bool is(node l, Q const& v) const { return (l != nil) && !CMP{}(v, val(l)); }

    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }

    static bndtree& nowhere() { static thread_local bndtree none; return none; }

    // The links of a node: to its children and, the one that the pool gives
    // to each object, to its parent. None of these fails on `nil`.
    node kid(node l, bool right) const
    {
        index i;
        return i.be(l.get()) ? kids[right].get(i) : node{ nil };
    }
    void set_kid(node l, bool right, node k)
    {
        index i;
        if (i.be(l.get())) {
            kids[right].set(i, k);
        }
    }
    node parent(node l) const
    {
        index i;
        return i.be(l.get()) ? this->link_of(i) : node{ nil };
    }
    void set_parent(node l, node p)
    {
        index i;
        if (i.be(l.get())) {
            this->link_to(i, p);
        }
    }
    // The height of the right subtree less that of the left one: -1, 0 or 1
    int leaning(node l) const
    {
        index i;
        return i.be(l.get()) ? lean.get(i) : 0;
    }
    void set_leaning(node l, int x)
    {
        index i;
        if (i.be(l.get())) {
            lean.set(i, static_cast<signed char>(x));
        }
    }

    // Makes `l` (which may be `nil`) the child of `above` on the given side;
    // the root, if `above` is `nil`
    void hang(node l, node above, bool right)
    {
        set_parent(l, above);
        if (above == nil) {
            root = l;
        }
        else {
            set_kid(above, right, l);
        }
    }

    // The leftmost or the rightmost node under (and including) `l`
    node furthest(node l, bool right) const
    {
        for (node k = kid(l, right); k != nil; k = kid(l, right)) {
            l = k;
        }
        return l;
    }
    // The node after (to the right) or before `l`, in order
    node beside(node l, bool right) const
    {
        if (!this->live_node(l)) {
            return nil;
        }
        if (kid(l, right) != nil) {
            return furthest(kid(l, right), !right);
        }
        node up = parent(l);
        while ((up != nil) && (l == kid(up, right))) {
            l  = up;
            up = parent(l);
        }
        return up;
    }
    // The height, by its leanings: of use only in a sound tree
    unsigned tall(node l) const
    {
        unsigned h = 0;
        for (; l != nil; l = kid(l, leaning(l) > 0)) {
            ++h;
        }
        return h;
    }

    // `z` is the child of `x` on the given side, and that side of `x` is
    // two higher than the other one. `z` does not lean the other way.
    // Makes `z` the top of the two and returns it; its parent is not set.
    node rotate(node x, node z, bool right)
    {
        const int  toward = right ? 1 : -1;
        const node inner  = kid(z, !right);
        set_kid(x, right, inner);
        set_parent(inner, x);
        set_kid(z, !right, x);
        set_parent(x, z);
        if (leaning(z) == 0) {      // only when erasing
            set_leaning(x, toward);
            set_leaning(z, -toward);
        }
        else {
            set_leaning(x, 0);
            set_leaning(z, 0);
        }
        return z;
    }
    // The same, but `z` leans the other way: its child there, `y`, is made
    // the top of the three and returned.
    node rotate_twice(node x, node z, bool right)
    {
        const int  toward = right ? 1 : -1;
        const node y      = kid(z, !right);
        const node far    = kid(y, right);
        set_kid(z, !right, far);
        set_parent(far, z);
        set_kid(y, right, z);
        set_parent(z, y);
        const node near = kid(y, !right);
        set_kid(x, right, near);
        set_parent(near, x);
        set_kid(y, !right, x);
        set_parent(x, y);
        if (leaning(y) == 0) {
            set_leaning(x, 0);
            set_leaning(z, 0);
        }
        else if (leaning(y) == toward) {
            set_leaning(x, -toward);
            set_leaning(z, 0);
        }
        else {
            set_leaning(x, 0);
            set_leaning(z, toward);
        }
        set_leaning(y, 0);
        return y;
    }

    // Takes the node out of the tree and destroys its element. No other
    // element moves: only the links change.
    void take_out(node gone)
    {
        const node above    = parent(gone);
        const bool on_right = (above != nil) && (gone == kid(above, true));
        node       x        = nil;      // the lowest node with a subtree that got lower
        bool       lower_on_right = false;

        if ((kid(gone, false) != nil) && (kid(gone, true) != nil)) {
            // The next one in order, which has no left child, takes the place
            const node next = furthest(kid(gone, true), false);
            if (next == kid(gone, true)) {
                x              = next;
                lower_on_right = true;
            }
            else {
                x              = parent(next);
                lower_on_right = false;
                hang(kid(next, true), x, false);
                set_kid(next, true, kid(gone, true));
                set_parent(kid(gone, true), next);
            }
            set_kid(next, false, kid(gone, false));
            set_parent(kid(gone, false), next);
            set_leaning(next, leaning(gone));
            hang(next, above, on_right);
        }
        else {
            const node only = (kid(gone, false) != nil) ? kid(gone, false) : kid(gone, true);
            hang(only, above, on_right);
            x              = above;
            lower_on_right = on_right;
        }
        this->drop_node(gone);

        // Up from there: the given side of `x` has become lower
        while (x != nil) {
            const node g       = parent(x);
            const bool g_right = (g != nil) && (x == kid(g, true));
            const int  toward  = lower_on_right ? 1 : -1;
            if (leaning(x) == -toward) {    // the other side is now two higher
                const node z    = kid(x, !lower_on_right);
                const int  zwas = leaning(z);
                hang((zwas == toward) ? rotate_twice(x, z, !lower_on_right) : rotate(x, z, !lower_on_right), g,
                     g_right);
                if (zwas == 0) {
                    break;      // as high as it was
                }
            }
            else if (leaning(x) == 0) {
                set_leaning(x, -toward);
                break;          // as high as it was
            }
            else {
                set_leaning(x, 0);
            }
            x              = g;
            lower_on_right = g_right;
        }
    }

    node                                    root;
    typename B::template array<node>        kids[2];    // left, right
    typename B::template array<signed char> lean;
};

template <class T, int N, class CMP = cobhlp::less<T>> using cobtree = bndtree<T, cobbound<N>, CMP>;

// The one with checked links
template <class T, int N, class CMP = cobhlp::less<T>> using cobcheckedtree = bndtree<T, cobbound<N>, CMP, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class T, int N, class CMP = cobhlp::less<T>>
using cobhivetree = bndtree<T, cobbound<N>, CMP, false, true>;
template <class T, int N, class CMP = cobhlp::less<T>>
using cobcheckedhivetree = bndtree<T, cobbound<N>, CMP, true, true>;


#endif // !defined(INC_COBTREE)
