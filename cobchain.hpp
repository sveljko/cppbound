/* What the compile time bound lists share. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBCHAIN)
#define      INC_COBCHAIN


#include "cobpool.hpp"

#include <utility>


// A pool whose objects are chained by their links, the first one being at
// `head`. Both the singly and the doubly linked list are such a chain.
// To the user, an element is at a `link`. With `CHECKED` being `true`, the
// links are checked handles of the pool: the link of an erased element is
// then the link of nothing, also after a new element takes its slot.
// With `HIVE` being `true`, which slots hold an element is kept as a bit each
// (see `cobhive`): an eighth of the room for that, a little more to ask.
template <class T, class B, bool CHECKED = false, bool HIVE = false>
struct bndchain : protected bndpool<T, B, CHECKED, HIVE> {
    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;

    using pool      = bndpool<T, B, CHECKED, HIVE>;
    using size_type = typename B::size_type;
    using link      = typename pool::handle;
    using index     = typename B::slot;

    static constexpr auto nil = B::nil;

    using pool::full;
    using pool::get;        // a copy of the element at the link, or `T{}`
    using pool::live;       // is there an element at the link? O(1).
    using pool::max_size;
    using pool::size;
    using pool::with;       // calls f with the element at the link, if there is one

    bool empty() const noexcept { return head == nil; }

    template <template <class> class V> V<T> front() const
    {
        return (head == nil) ? V<T>{} : V<T>(val(head));
    }

    link lbegin() const { return this->ticket(head); }
    link lend() const { return nil; }
    link lfront() const { return this->ticket(head); }

    link fwd(link l) const { return this->ticket(after(this->raw(l))); }
    bool contains(link l) const { return live(l); }

    bool set(link l, T const& t)
    {
        return with(l, [&](T& x) { x = t; });
    }
    bool set(link l, T&& t)
    {
        return with(l, [&](T& x) { x = std::move(t); });
    }

    // Calls f(T&) for each element, first to last
    template <class F> void for_each(F&& f)
    {
        for (node l = head; l != nil; l = after(l)) {
            f(val(l));
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (node l = head; l != nil; l = after(l)) {
            f(val(l));
        }
    }

protected:
    // Inside, an element is at a node
    using typename pool::node;

    bndchain() : head(nil) {}

    node after(node l) const
    {
        index i;
        return i.be(l.get()) ? this->link_of(i) : node{ nil };
    }

    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }

    T&       val(node l) { index i; i.be(l.get()); return this->at(i); }
    T const& val(node l) const { index i; i.be(l.get()); return this->at(i); }

    // What an iterator at `l` refers to
    T& target(link l)
    {
        const node n = this->raw(l);
        index      i;
        return (i.be(n.get()) && this->live_node(n)) ? this->at(i) : spot();
    }
    T const& target(link l) const
    {
        const node n = this->raw(l);
        index      i;
        return (i.be(n.get()) && this->live_node(n)) ? this->at(i) : spot();
    }

    void link_after(node l, node n)
    {
        index i;
        if (i.be(l.get())) {
            this->link_to(i, n);
        }
    }

    // Sorts the n nodes that start at h, returns the first node of the result,
    // the last node of which is followed by nil. Stable; changes links only.
    template <class C> node msort(node h, size_type n, C& comp)
    {
        if (n < 2) {
            link_after(h, nil);
            return h;
        }
        node mid = h;
        for (size_type i = 0; i < n / 2; ++i) { mid = after(mid); }   // before the first half is cut off
        node a     = msort(h, n / 2, comp);
        node b     = msort(mid, n - n / 2, comp);
        node first = nil;
        node last  = nil;
        while ((a != nil) || (b != nil)) {
            node take = nil;
            if ((b == nil) || ((a != nil) && !comp(val(b), val(a)))) {
                take = a;
                a    = after(a);
            }
            else {
                take = b;
                b    = after(b);
            }
            if (last == nil) { first = take; } else { link_after(last, take); }
            last = take;
        }
        link_after(last, nil);
        return first;
    }

    node head;
};


#endif // !defined(INC_COBCHAIN)
