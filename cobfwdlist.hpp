/* Compile time bound singly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFWDLIST)
#define      INC_COBFWDLIST


#include "cobarray.hpp"
#include "cobstore.hpp"

#include <utility>


template <class T, int N>
struct cobfwdlist {
    static_assert(N > 0, "List must have some capacity");

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    using link = cobint<0, N>;
    using index = cobint<0, N-1>;

    static constexpr auto nil = cobic<N>;

    cobfwdlist() : head(nil), vacant(cobic<0>) {
        int i = 1;
        for (auto& x: next) {
            if (!x.be(i++)) {
                x = nil;
            }
        }
        used.fill(false);
    }

    constexpr bool empty() const noexcept { return head == nil; }
    constexpr unsigned size() const noexcept { return count; }
    constexpr bool full() const noexcept { return count == N; }
    constexpr unsigned max_size() const noexcept { return N; }

    template <class... A> constexpr bool emplace_front(A&&... a) {
        if (nil == vacant) {
            return false;
        }
        index vacant_idx;
        vacant_idx.be(vacant.get());
        const link l = vacant;
        vacant = next.get(vacant_idx);
        ++count;

        next.set(vacant_idx, head);
        used.set(vacant_idx, true);
        head = l;

        store.make(vacant_idx, std::forward<A>(a)...);

        return true;
    }
    constexpr bool push_front(T const& t) { return emplace_front(t); }
    constexpr bool push_front(T&& t) { return emplace_front(std::move(t)); }

    template<template<class> class V> constexpr V<T> pop_front() {
        if (head == nil) {
            return {};
        }
        index head_idx;
        head_idx.be(head.get());
        const auto l = head;
        V<T> rslt(std::move(store.at(head_idx)));
        store.drop(head_idx);
        head = next.get(head_idx);

        next.set(head_idx, vacant);
        used.set(head_idx, false);
        vacant = l;
        --count;

        return rslt;
    }

    class I {
        cobfwdlist * r;
        link l;
    public:
        I() : r(&nowhere()), l(nil) {}
        I operator++(int) { I t = *this; ++*this; return t; }
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;

        I& operator++() {
            l = r->fwd(l);
            return *this;
        }

        T& operator*() const { index i; return (i.be(l.get()) && r->live(l)) ? r->store.at(i) : spot(); }
        T* operator->() const { return &**this; }

        constexpr bool operator==(I const& x) const { return (r == x.r) && (l == x.l); }

        constexpr int idx() {return l.get();}

        friend struct cobfwdlist;

    protected:
        I(cobfwdlist const* r_, link l_) : r(const_cast<cobfwdlist*>(r_)), l(l_) {}
    };

    class CI {
        cobfwdlist const* r;
        link l;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T const*;
        using reference = T const&;
        using iterator_category = std::forward_iterator_tag;
       
        CI() : r(&nowhere()), l(nil) {}
        CI operator++(int) { CI t = *this; ++*this; return t; }
        CI(I const& x) : r(x.r), l(x.l) {}

        CI& operator++() {
            l = r->fwd(l);
            return *this;
        }

        T const& operator*() const { index i; return (i.be(l.get()) && r->live(l)) ? r->store.at(i) : spot(); }
        T const* operator->() const { return &**this; }

        bool operator==(CI const& x) const { return (r == x.r) && (l == x.l); }

        friend struct cobfwdlist;

    protected:
        CI(cobfwdlist const* r_, link l_) : r(r_), l(l_) {}
    };

    using iterator = I;
    using const_iterator = CI;

    constexpr I begin() { return I{this, head}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head}; }
    constexpr CI cend() const { return CI{this, nil}; }
    // TODO? before_begin()?

    template <class... A> constexpr link emplace_after(link pos, A&&... a) {
        if (nil == vacant) {
            return nil;
        }
        if (!live(pos)) {
            return nil;
        }
        index pos_idx;
        pos_idx.be(pos.get());
        index vacant_idx;
        vacant_idx.be(vacant.get());
        vacant = next.get(vacant_idx);
        ++count;

        next.set(vacant_idx, next.get(pos_idx));
        next.set(pos_idx, vacant_idx);
        used.set(vacant_idx, true);

        store.make(vacant_idx, std::forward<A>(a)...);

        return vacant_idx;
    }
    constexpr link insert_after(link pos, T const& value) { return emplace_after(pos, value); }
    constexpr link insert_after(link pos, T&& value) { return emplace_after(pos, std::move(value)); }
    constexpr I insert_after(CI pos, T const& value) { return I{this, emplace_after(pos.l, value)}; }
    constexpr I insert_after(CI pos, T&& value) { return I{this, emplace_after(pos.l, std::move(value))}; }

    constexpr link erase_after(link pos) {
        return erase_after(CI{this, pos}).l;
    }
    constexpr I erase_after(CI pos) {
        if (!live(pos.l)) {
            return I{this, nil};
        }
        index pos_idx;
        pos_idx.be(pos.l.get());

        link after = fwd(pos.l);
        if (nil == after) {
            return I{this, nil};
        }
        index next_idx;
        next_idx.be(after.get());

        store.drop(next_idx);
        next.set(pos_idx, next.get(next_idx));
        next.set(next_idx, vacant);
        used.set(next_idx, false);
        vacant = after;
        --count;

        return I{this, next.get(pos_idx)};
    }


    template <template <class> class V> constexpr V<T> front() const {
        return (head == nil) ? V<T>{} : V<T>(val(head));
    }

    // Both lists are sorted. Moves the elements of x into this, keeping the order.
    constexpr bool merge(cobfwdlist& x) {
        return merge(x, [](T const& t, T const& u) { return t < u; });
    }
    template <class C> constexpr bool merge(cobfwdlist& x, C comp) {
        if ((&x == this) || (size() + x.size() > N)) {
            return false;
        }
        link before = nil;
        link p = head;
        for (link l = x.head; l != nil; l = x.fwd(l)) {
            while ((p != nil) && !comp(x.val(l), val(p))) {
                before = p;
                p = fwd(p);
            }
            if (before == nil) {
                emplace_front(std::move(x.val(l)));
                before = head;
            }
            else {
                before = emplace_after(before, std::move(x.val(l)));
            }
        }
        x.clear();
        return true;
    }

    constexpr void swap(cobfwdlist& x) {
        cobfwdlist t = *this;
        *this = x;
        x = t;
    }

    constexpr void clear() {
        while (!empty()) {
            pop_front<ignore>();
        }
    }

    // TODO splice(pos, list&), splice(pos, list&, it), splice(pos, list&, first, last)

    constexpr unsigned remove(T const& t) {
        return remove_if([&](T const& x) { return x == t;});
    }

    template <class UP> constexpr unsigned remove_if(UP p) {
        unsigned rslt = 0;
        while (!empty() && p(val(head))) {
            pop_front<ignore>();
            ++rslt;
        }
        if (empty()) {
            return rslt;
        }
        link l = head;
        link after = fwd(head);
        while (after != nil) {
            if (p(val(after))) {
                erase_after(l);
                ++rslt;
            }
            else {
                l = after;
            }
            after = fwd(l);
        }
        return rslt;
    }

    constexpr unsigned unique() {
        return unique([](T const& t, T const& u) { return t == u;});
    }

    template <class BP> constexpr unsigned unique(BP p) {
        link f = head;
        link after = fwd(head);
        unsigned rslt = 0;
        while (after != nil) {
            if (p(val(after), val(f))) {
                erase_after(f);
                ++rslt;
            }
            else {
                f = fwd(f);
            }
            after = fwd(f);
        }
        return rslt;
    }

    constexpr void reverse() {
        link l = head;
        link before = nil;
        while (l != nil) {
            link after = fwd(l);
            index i;
            i.be(l.get());
            next.set(i, before);
            before = l;
            l = after;
        }
        head = before;
    }

    constexpr void sort() {
        sort([](T const& t, T const& u) { return t < u;});
    }

    template <class C> constexpr void sort(C comp) {
        if (size() < 2) { return; }
        head = msort(head, size(), comp);
    }

    constexpr link lbegin() const { return head; }
    constexpr link lend() const { return nil; }
    constexpr link lfront() const { return head; }

    constexpr link fwd(link l) const {
        index lidx;
        if (lidx.be(l.get())) {
            return next.get(lidx);
        }
        else {
            return nil;
        }
    }
    // Is there an element at `l`? O(1).
    constexpr bool live(link l) const {
        index i;
        return i.be(l.get()) && used.get(i);
    }
    constexpr bool contains(link l) const {
        return live(l);
    }
    constexpr bool contains(CI pos) const {
        return (pos.r == this) && contains(pos.l);
    }

    constexpr T get(link l) const {
        index i;
        return (i.be(l.get()) && live(l)) ? store.at(i) : T{};
    }
    constexpr bool set(link l, T const& t) {
        return with(l, [&](T& x) { x = t; });
    }
    constexpr bool set(link l, T&& t) {
        return with(l, [&](T& x) { x = std::move(t); });
    }

    // Calls f with the element at l, if there is one. Returns whether it did.
    template <class F> constexpr bool with(link l, F&& f) {
        index i;
        if (i.be(l.get()) && live(l)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    template <class F> constexpr bool with(link l, F&& f) const {
        index i;
        if (i.be(l.get()) && live(l)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    // Calls f(T&) for each element, first to last
    template <class F> constexpr void for_each(F&& f) {
        for (link l = head; l != nil; l = fwd(l)) {
            f(val(l));
        }
    }
    template <class F> constexpr void for_each(F&& f) const {
        for (link l = head; l != nil; l = fwd(l)) {
            f(val(l));
        }
    }

    cobfwdlist(cobfwdlist const& x) : head(x.head), next(x.next), used(x.used), vacant(x.vacant), count(x.count) {
        copy_elements(x);
    }
    cobfwdlist& operator=(cobfwdlist const& x) {
        if (this != &x) {
            clear();
            head = x.head; next = x.next; used = x.used; vacant = x.vacant; count = x.count;
            copy_elements(x);
        }
        return *this;
    }
    ~cobfwdlist() { clear(); }

private:
    static cobfwdlist& nowhere() { static thread_local cobfwdlist none{}; return none; }

    template <class U> struct ignore {
        ignore() {}
        ignore(U const&) {}
    };
    
    constexpr void link_after(link l, link n) { index i; if (i.be(l.get())) { next.set(i, n); } }

    // Sorts the n nodes that start at h, returns the first node of the result,
    // the last node of which is followed by nil.
    template <class C> constexpr link msort(link h, unsigned n, C& comp) {
        if (n < 2) {
            link_after(h, nil);
            return h;
        }
        link mid = h;
        for (unsigned i = 0; i < n / 2; ++i) { mid = fwd(mid); }   // before the first half is cut off
        link a = msort(h, n / 2, comp);
        link b = msort(mid, n - n / 2, comp);
        link first = nil;
        link last = nil;
        while ((a != nil) || (b != nil)) {
            link take = nil;
            if ((b == nil) || ((a != nil) && !comp(val(b), val(a)))) {
                take = a;
                a = fwd(a);
            }
            else {
                take = b;
                b = fwd(b);
            }
            if (last == nil) { first = take; } else { link_after(last, take); }
            last = take;
        }
        link_after(last, nil);
        return first;
    }

    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }

    constexpr T& val(link l) { index i; i.be(l.get()); return store.at(i); }
    constexpr T const& val(link l) const { index i; i.be(l.get()); return store.at(i); }

    constexpr void copy_elements(cobfwdlist const& x) {
        for (link l = head; l != nil; l = fwd(l)) {
            index i;
            if (i.be(l.get())) { store.make(i, x.store.at(i)); }
        }
    }

    cobstore<T, N> store;
    link head;
    cobarray<link, N> next;
    cobarray<bool, N> used;
    link vacant;
    unsigned count = 0;
};


#endif // define      INC_COBFWDLIST
