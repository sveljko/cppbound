/* Compile time bound singly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFWDLIST)
#define      INC_COBFWDLIST


#include "cobchain.hpp"

#include <iterator>
#include <utility>


template <class T, class B, bool CHECKED = false, bool HIVE = false>
struct bndfwdlist : bndchain<T, B, CHECKED, HIVE> {
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

    bndfwdlist() {}

    template <class... A> bool emplace_front(A&&... a)
    {
        const node l = this->make_node(std::forward<A>(a)...);
        index      i;
        if (!i.be(l.get())) {
            return false;
        }
        this->link_to(i, head);
        head = l;
        return true;
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
        drop_front();
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
        using iterator_category = std::forward_iterator_tag;

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

        U& operator*() const { return r->target(l); }
        U* operator->() const { return &**this; }

        bool operator==(iter const& x) const { return (r == x.r) && (l == x.l); }

        // The number of the slot of the element; that of no slot at the end
        auto idx() const { return r->raw(l).get(); }

        friend struct bndfwdlist;
        template <class, class> friend class iter;

    private:
        iter(L* r_, link l_) : r(r_), l(l_) {}
    };
    using I              = iter<bndfwdlist, T>;
    using CI             = iter<bndfwdlist const, T const>;
    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, this->lbegin() }; }
    I  end() { return I{ this, nil }; }
    CI begin() const { return CI{ this, this->lbegin() }; }
    CI end() const { return CI{ this, nil }; }
    CI cbegin() const { return CI{ this, this->lbegin() }; }
    CI cend() const { return CI{ this, nil }; }
    // TODO? before_begin()?

    // Returns the link of the new element, `nil` if there is no room or no
    // element at `pos`.
    template <class... A> link emplace_after(link pos, A&&... a)
    {
        if (!live(pos)) {
            return nil;
        }
        return this->ticket(emplace_past(this->raw(pos), std::forward<A>(a)...));
    }
    link insert_after(link pos, T const& value) { return emplace_after(pos, value); }
    link insert_after(link pos, T&& value) { return emplace_after(pos, std::move(value)); }
    I    insert_after(CI pos, T const& value) { return I{ this, emplace_after(pos.l, value) }; }
    I    insert_after(CI pos, T&& value) { return I{ this, emplace_after(pos.l, std::move(value)) }; }

    // Returns the link following the erased one; `nil` also if nothing was erased
    link erase_after(link pos)
    {
        if (!live(pos)) {
            return nil;
        }
        return this->ticket(cut_past(this->raw(pos)));
    }
    I erase_after(CI pos) { return I{ this, erase_after(pos.l) }; }

    // Both lists are sorted. Moves the elements of x into this, keeping the order.
    bool merge(bndfwdlist& x)
    {
        return merge(x, [](T const& t, T const& u) { return t < u; });
    }
    template <class C> bool merge(bndfwdlist& x, C comp)
    {
        if ((&x == this) || (x.size() > max_size() - size())) {
            return false;
        }
        node last = nil;
        node p    = head;
        for (node l = x.head; l != nil; l = x.after(l)) {
            while ((p != nil) && !comp(x.val(l), this->val(p))) {
                last = p;
                p    = this->after(p);
            }
            if (last == nil) {
                emplace_front(std::move(x.val(l)));
                last = head;
            }
            else {
                last = emplace_past(last, std::move(x.val(l)));
            }
        }
        x.clear();
        return true;
    }

    void swap(bndfwdlist& x)
    {
        bndfwdlist t = *this;
        *this        = x;
        x            = t;
    }

    void clear()
    {
        chain::pool::clear();
        head = nil;
    }

    // TODO splice_after(pos, list&), splice_after(pos, list&, it), splice_after(pos, list&, first, last)

    size_type remove(T const& t)
    {
        return remove_if([&](T const& x) { return x == t; });
    }
    template <class UP> size_type remove_if(UP p)
    {
        size_type rslt = 0;
        while (!empty() && p(this->val(head))) {
            drop_front();
            ++rslt;
        }
        if (empty()) {
            return rslt;
        }
        node l    = head;
        node next = this->after(head);
        while (next != nil) {
            if (p(this->val(next))) {
                cut_past(l);
                ++rslt;
            }
            else {
                l = next;
            }
            next = this->after(l);
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
        node      next = this->after(head);
        size_type rslt = 0;
        while (next != nil) {
            if (p(this->val(next), this->val(f))) {
                cut_past(f);
                ++rslt;
            }
            else {
                f = this->after(f);
            }
            next = this->after(f);
        }
        return rslt;
    }

    void reverse()
    {
        node l    = head;
        node last = nil;
        while (l != nil) {
            const node next = this->after(l);
            this->link_after(l, last);
            last = l;
            l    = next;
        }
        head = last;
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
        head = this->msort(head, size(), comp);
    }

    using chain::contains;
    bool contains(CI pos) const { return (pos.r == this) && live(pos.l); }

private:
    using typename chain::node;
    using chain::head;

    static bndfwdlist& nowhere() { static thread_local bndfwdlist none{}; return none; }

    void drop_front()
    {
        index i;
        if (i.be(head.get())) {
            head = this->link_of(i);
            this->drop_node(i);
        }
    }
    // After the node `pos`, which is in the list
    template <class... A> node emplace_past(node pos, A&&... a)
    {
        index at;
        if (!at.be(pos.get())) {
            return nil;
        }
        const node l = this->make_node(std::forward<A>(a)...);
        index      i;
        if (!i.be(l.get())) {
            return nil;
        }
        this->link_to(i, this->link_of(at));
        this->link_to(at, l);
        return l;
    }
    // Takes out what is after the node `pos`, which is in the list.
    // Returns what follows that, `nil` also if there was nothing to take out.
    node cut_past(node pos)
    {
        index at;
        index gone;
        if (!at.be(pos.get()) || !gone.be(this->link_of(at).get())) {
            return nil;
        }
        const node next = this->link_of(gone);
        this->link_to(at, next);
        this->drop_node(gone);
        return next;
    }
};

template <class T, int N> using cobfwdlist        = bndfwdlist<T, cobbound<N>>;
// The one with checked links
template <class T, int N> using cobcheckedfwdlist = bndfwdlist<T, cobbound<N>, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class T, int N>
using cobhivefwdlist = bndfwdlist<T, cobbound<N>, false, true>;
template <class T, int N>
using cobcheckedhivefwdlist = bndfwdlist<T, cobbound<N>, true, true>;


#endif // define      INC_COBFWDLIST
