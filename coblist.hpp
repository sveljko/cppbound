/* Compile time bound doubly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBLIST)
#define      INC_COBLIST


#include "cobchain.hpp"

#include <iterator>
#include <utility>


template <class T, class B, bool CHECKED = false, bool HIVE = false>
struct bndlist : bndchain<T, B, CHECKED, HIVE> {
    using chain     = bndchain<T, B, CHECKED, HIVE>;
    using size_type = typename chain::size_type;
    using link      = typename chain::link;
    using index     = typename chain::index;

    using chain::nil;
    using chain::empty;
    using chain::fwd;
    using chain::live;
    using chain::max_size;
    using chain::size;

    bndlist() : tail(nil) {}

    template <class... A> bool emplace_front(A&&... a)
    {
        return emplace_at(head, std::forward<A>(a)...) != nil;
    }
    bool push_front(T const& t) { return emplace_front(t); }
    bool push_front(T&& t) { return emplace_front(std::move(t)); }

    template <template <class> class V> V<T> pop_front()
    {
        index i;
        if (!i.be(head.get())) {
            return {};
        }
        V<T> rslt(std::move(this->at(i)));
        cut(i);
        return rslt;
    }

    template <class... A> bool emplace_back(A&&... a)
    {
        return emplace_at(nil, std::forward<A>(a)...) != nil;
    }
    bool push_back(T const& t) { return emplace_back(t); }
    bool push_back(T&& t) { return emplace_back(std::move(t)); }

    template <template <class> class V> V<T> pop_back()
    {
        index i;
        if (!i.be(tail.get())) {
            return {};
        }
        V<T> rslt(std::move(this->at(i)));
        cut(i);
        return rslt;
    }

    // The iterators differ in constness only: `L` is the list, `U` the element
    template <class L, class U> class iter {
        L*   r;
        link l;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = U*;
        using reference         = U&;
        using iterator_category = std::bidirectional_iterator_tag;

        iter() : r(&nowhere()), l(nil) {}
        // A constant iterator from one that is not
        template <class L2, class U2>
            requires(std::is_const_v<L> && !std::is_const_v<L2>)
        iter(iter<L2, U2> const& x) : r(x.r), l(x.l) {}

        iter& operator++()
        {
            l = r->fwd(l);
            return *this;
        }
        iter operator++(int) { iter t = *this; ++*this; return t; }
        iter& operator--()
        {
            l = (l == nil) ? r->lback() : r->bck(l);
            return *this;
        }
        iter operator--(int) { iter t = *this; --*this; return t; }

        U& operator*() const { return r->target(l); }
        U* operator->() const { return &**this; }

        bool operator==(iter const& x) const { return (r == x.r) && (l == x.l); }

        friend struct bndlist;
        template <class, class> friend class iter;

    private:
        iter(L* r_, link l_) : r(r_), l(l_) {}
    };
    using I              = iter<bndlist, T>;
    using CI             = iter<bndlist const, T const>;
    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, this->lbegin() }; }
    I  end() { return I{ this, nil }; }
    CI begin() const { return CI{ this, this->lbegin() }; }
    CI end() const { return CI{ this, nil }; }
    CI cbegin() const { return CI{ this, this->lbegin() }; }
    CI cend() const { return CI{ this, nil }; }
    // TODO rbegin(), rend(), crbegin(), crend()

    // Before `pos`; at the end if `pos` is `nil`. Returns the link of the
    // new element, `nil` if there is no room or no element at `pos`.
    template <class... A> link emplace(link pos, A&&... a)
    {
        if ((pos != nil) && !live(pos)) {
            return nil;
        }
        return this->ticket(emplace_at(this->raw(pos), std::forward<A>(a)...));
    }
    link insert(link pos, T const& value) { return emplace(pos, value); }
    link insert(link pos, T&& value) { return emplace(pos, std::move(value)); }
    I    insert(CI pos, T const& value) { return I{ this, emplace(pos.l, value) }; }
    I    insert(CI pos, T&& value) { return I{ this, emplace(pos.l, std::move(value)) }; }

    // Returns the link following the erased one; `nil` also if there is no
    // element at `pos` (in which case nothing is erased).
    link erase(link pos)
    {
        const node n = this->raw(pos);
        index      i;
        if (!i.be(n.get()) || !live(pos)) {
            return nil;
        }
        const node next = this->link_of(i);
        cut(i);
        return this->ticket(next);
    }
    I erase(CI pos) { return I{ this, erase(pos.l) }; }

    template <template <class> class V> V<T> back() const
    {
        return (tail == nil) ? V<T>{} : V<T>(this->val(tail));
    }

    // Moves all elements of x before pos, in order, leaving x empty.
    bool splice(link pos, bndlist& x)
    {
        if ((&x == this) || (x.size() > max_size() - size()) || ((pos != nil) && !live(pos))) {
            return false;
        }
        const node at = this->raw(pos);
        for (node l = x.head; l != nil; l = x.after(l)) {
            emplace_at(at, std::move(x.val(l)));
        }
        x.clear();
        return true;
    }

    // Both lists are sorted. Moves the elements of x into this, keeping the order.
    bool merge(bndlist& x)
    {
        return merge(x, [](T const& t, T const& u) { return t < u; });
    }
    template <class C> bool merge(bndlist& x, C comp)
    {
        if ((&x == this) || (x.size() > max_size() - size())) {
            return false;
        }
        node p = head;
        for (node l = x.head; l != nil; l = x.after(l)) {
            while ((p != nil) && !comp(x.val(l), this->val(p))) {
                p = this->after(p);
            }
            emplace_at(p, std::move(x.val(l)));
        }
        x.clear();
        return true;
    }

    void swap(bndlist& x)
    {
        bndlist t = *this;
        *this     = x;
        x         = t;
    }

    void clear()
    {
        chain::pool::clear();
        head = tail = nil;
    }

    // TODO splice(pos, list&, it), splice(pos, list&, first, last)

    size_type remove(T const& t)
    {
        return remove_if([&](T const& x) { return x == t; });
    }
    template <class UP> size_type remove_if(UP p)
    {
        node      l    = head;
        size_type rslt = 0;
        while (l != nil) {
            const node n = this->after(l);
            if (p(this->val(l))) {
                cut(l);
                ++rslt;
            }
            l = n;
        }
        return rslt;
    }

    size_type unique()
    {
        return unique([](T const& t, T const& u) { return t == u; });
    }
    template <class BP> size_type unique(BP p)
    {
        node      f    = head;
        node      l    = this->after(head);
        size_type rslt = 0;
        while (l != nil) {
            const node n = this->after(l);
            if (p(this->val(l), this->val(f))) {
                cut(l);
                ++rslt;
            }
            else {
                f = l;
            }
            l = n;
        }
        return rslt;
    }

    void reverse()
    {
        node l = head;
        node t = tail;
        while (l != t) {
            std::swap(this->val(l), this->val(t));
            l = this->after(l);
            if (l == t) {
                break;
            }
            t = before(t);
        }
    }

    void sort()
    {
        sort([](T const& t, T const& u) { return t < u; });
    }
    template <class C> void sort(C comp)
    {
        if (size() < 2) {
            return;
        }
        head   = this->msort(head, size(), comp);
        node p = nil;                       // the forward links are right, redo the rest
        for (node l = head; l != nil; l = this->after(l)) {
            index i;
            if (i.be(l.get())) { prev.set(i, p); }
            p = l;
        }
        tail = p;
    }

    link lback() const { return this->ticket(tail); }

    link bck(link l) const { return this->ticket(before(this->raw(l))); }
    using chain::contains;
    bool contains(CI pos) const { return (pos.r == this) && live(pos.l); }

private:
    using typename chain::node;
    using chain::head;

    static bndlist& nowhere() { static thread_local bndlist none{}; return none; }

    node before(node l) const
    {
        index i;
        return i.be(l.get()) ? prev.get(i) : node{ nil };
    }

    // Before the node `pos`, which is in the list, or at the end if it is `nil`
    template <class... A> node emplace_at(node pos, A&&... a)
    {
        const node l = this->make_node(std::forward<A>(a)...);
        index      i;
        if (!i.be(l.get())) {
            return nil;
        }
        const node last = (pos == nil) ? tail : before(pos);
        this->link_to(i, pos);
        prev.set(i, last);
        index x;
        if (x.be(last.get())) { this->link_to(x, l); } else { head = l; }
        if (x.be(pos.get())) { prev.set(x, l); } else { tail = l; }
        return l;
    }

    // Takes the element out of the list and destroys it
    void cut(node l)
    {
        index i;
        if (i.be(l.get())) {
            cut(i);
        }
    }
    void cut(index i)
    {
        const node next = this->link_of(i);
        const node last = prev.get(i);
        index      x;
        if (x.be(last.get())) { this->link_to(x, next); } else { head = next; }
        if (x.be(next.get())) { prev.set(x, last); } else { tail = last; }
        this->drop_node(i);
    }

    node                             tail;
    typename B::template array<node> prev;
};

template <class T, int N> using coblist        = bndlist<T, cobbound<N>>;
// The one with checked links
template <class T, int N> using cobcheckedlist = bndlist<T, cobbound<N>, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class T, int N>
using cobhivelist = bndlist<T, cobbound<N>, false, true>;
template <class T, int N>
using cobcheckedhivelist = bndlist<T, cobbound<N>, true, true>;


#endif // define      INC_COBLIST
