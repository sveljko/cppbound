/* Compile time bound doubly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBLIST)
#define      INC_COBLIST


#include "cobarray.hpp"
#include "cobstore.hpp"

#include <utility>


template <class T, int N>
struct coblist {
    static_assert(N > 0, "List must have some capacity");

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    using link = cobint<0, N>;
    using index = cobint<0, N-1>;

    static constexpr auto nil = cobic<N>;

    coblist() : head(nil), tail(nil), vacant(cobic<0>) {
        int i = 1;
        for (auto& x: next) {
            if (!x.be(i++)) {
                x = nil;
            }
        }
        for (auto i : prev.irange()) { prev.set(i, i); }
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
        if (head != nil) {
            index head_idx;
            head_idx.be(head.get());
            prev.set(head_idx, l);
            head = l;
        }
        else {
            head = tail = l;
        }
        prev.set(vacant_idx, nil);

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
        if (head != nil) {
            index new_head_idx;
            new_head_idx.be(head.get());
            prev.set(new_head_idx, nil);
        }
        else {
            tail = nil;
        }

        next.set(head_idx, vacant);
        prev.set(head_idx, head_idx);
        vacant = l;
        --count;

        return rslt;
    }

    template <class... A> constexpr bool emplace_back(A&&... a) {
        if (nil == vacant) {
            return false;
        }
        index vacant_idx;
        vacant_idx.be(vacant.get());
        const link l = vacant;
        vacant = next.get(vacant_idx);
        ++count;

        prev.set(vacant_idx, tail);
        if (tail != nil) {
            index tail_idx;
            tail_idx.be(tail.get());
            next.set(tail_idx, l);
            tail = l;
        }
        else {
            head = tail = l;
        }
        next.set(vacant_idx, nil);

        store.make(vacant_idx, std::forward<A>(a)...);

        return true;
    }
    constexpr bool push_back(T const& t) { return emplace_back(t); }
    constexpr bool push_back(T&& t) { return emplace_back(std::move(t)); }

    template<template<class> class V> constexpr V<T> pop_back() {
        if (tail == nil) {
            return {};
        }
        index tail_idx;
        tail_idx.be(tail.get());
        const auto l = tail;
        V<T> rslt(std::move(store.at(tail_idx)));
        store.drop(tail_idx);
        tail = prev.get(tail_idx);
        if (tail != nil) {
            index new_tail_idx;
            new_tail_idx.be(tail.get());
            next.set(new_tail_idx, nil);
        }
        else {
            head = nil;
        }

        next.set(tail_idx, vacant);
        prev.set(tail_idx, tail_idx);
        vacant = l;
        --count;

        return rslt;
    }

    class I {
        coblist * r;
        link l;
    public:
        I() : r(&nowhere()), l(nil) {}
        I operator++(int) { I t = *this; ++*this; return t; }
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;

        I& operator++() {
            l = r->fwd(l);
            return *this;
        }
        I& operator--() {
            l = (l == nil) ? r->tail : r->bck(l);
            return *this;
        }
        I operator--(int) { I t = *this; --*this; return t; }

        T& operator*() const { index i; return (i.be(l.get()) && r->live(l)) ? r->store.at(i) : spot(); }
        T* operator->() const { return &**this; }

        bool operator==(I const& x) const { return (r == x.r) && (l == x.l); }

        friend struct coblist;

    protected:
        I(coblist const* r_, link l_) : r(const_cast<coblist*>(r_)), l(l_) {}
    };

    class CI {
        coblist const* r;
        link l;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T const*;
        using reference = T const&;
        using iterator_category = std::bidirectional_iterator_tag;
       
        CI() : r(&nowhere()), l(nil) {}
        CI operator++(int) { CI t = *this; ++*this; return t; }
        CI(I const& x) : r(x.r), l(x.l) {}

        CI& operator++() {
            l = r->fwd(l);
            return *this;
        }
        CI& operator--() {
            l = (l == nil) ? r->tail : r->bck(l);
            return *this;
        }
        CI operator--(int) { CI t = *this; --*this; return t; }

        T const& operator*() const { index i; return (i.be(l.get()) && r->live(l)) ? r->store.at(i) : spot(); }
        T const* operator->() const { return &**this; }

        bool operator==(CI const& x) const { return (r == x.r) && (l == x.l); }

        friend struct coblist;

    protected:
        CI(coblist const* r_, link l_) : r(r_), l(l_) {}
    };

    using iterator = I;
    using const_iterator = CI;

    constexpr I begin() { return I{this, head}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head}; }
    constexpr CI cend() const { return CI{this, nil}; }
    // TODO rbegin(), rend(), crbegin(), crend()

    template <class... A> constexpr link emplace(link pos, A&&... a) {
        if (nil == vacant) {
            return nil;
        }
        if ((pos != nil) && !live(pos)) {
            return nil;
        }
        if (nil == pos) {
            emplace_back(std::forward<A>(a)...);
            return tail;
        }
        if (pos == head) {
            emplace_front(std::forward<A>(a)...);
            return head;
        }
        index pos_idx;
        pos_idx.be(pos.get());
        index vacant_idx;
        vacant_idx.be(vacant.get());
        vacant = next.get(vacant_idx);
        ++count;

        index prev_idx;
        prev_idx.be(bck(pos).get());
        next.set(prev_idx, vacant_idx);
        next.set(vacant_idx, pos);
        prev.set(vacant_idx, prev_idx);
        prev.set(pos_idx, vacant_idx);

        store.make(vacant_idx, std::forward<A>(a)...);

        return vacant_idx;
    }
    constexpr link insert(link pos, T const& value) { return emplace(pos, value); }
    constexpr link insert(link pos, T&& value) { return emplace(pos, std::move(value)); }
    constexpr I insert(CI pos, T const& value) { return I{this, emplace(pos.l, value)}; }
    constexpr I insert(CI pos, T&& value) { return I{this, emplace(pos.l, std::move(value))}; }

    constexpr link erase(link pos) {
        return erase(CI{this, pos}).l;
    }
    constexpr I erase(CI pos) {
        if (!live(pos.l)) {
            return I{this, nil};
        }
        if (pos.l == tail) {
            pop_back<ignore>();
            return I{this, nil};
        }
        if (pos.l == head) {
            pop_front<ignore>();
            return I{this, head};
        }
        index pos_idx;
        pos_idx.be(pos.l.get());

        index prev_idx;
        prev_idx.be(bck(pos.l).get());
        next.set(prev_idx, next.get(pos_idx));

        index next_idx;
        next_idx.be(fwd(pos.l).get());
        prev.set(next_idx, prev.get(pos_idx));

        store.drop(pos_idx);
        next.set(pos_idx, vacant);
        prev.set(pos_idx, pos_idx);
        vacant = pos.l;
        --count;

        return I{this, next_idx};
    }


    template <template <class> class V> constexpr V<T> front() const {
        return (head == nil) ? V<T>{} : V<T>(val(head));
    }
    template <template <class> class V> constexpr V<T> back() const {
        return (tail == nil) ? V<T>{} : V<T>(val(tail));
    }

    // Moves all elements of x before pos, in order, leaving x empty.
    constexpr bool splice(link pos, coblist& x) {
        if ((&x == this) || (size() + x.size() > N) || ((pos != nil) && !live(pos))) {
            return false;
        }
        for (link l = x.head; l != nil; l = x.fwd(l)) {
            emplace(pos, std::move(x.val(l)));
        }
        x.clear();
        return true;
    }

    // Both lists are sorted. Moves the elements of x into this, keeping the order.
    constexpr bool merge(coblist& x) {
        return merge(x, [](T const& t, T const& u) { return t < u; });
    }
    template <class C> constexpr bool merge(coblist& x, C comp) {
        if ((&x == this) || (size() + x.size() > N)) {
            return false;
        }
        link p = head;
        for (link l = x.head; l != nil; l = x.fwd(l)) {
            while ((p != nil) && !comp(x.val(l), val(p))) {
                p = fwd(p);
            }
            emplace(p, std::move(x.val(l)));
        }
        x.clear();
        return true;
    }

    constexpr void swap(coblist& x) {
        coblist t = *this;
        *this = x;
        x = t;
    }

    constexpr void clear() {
        while (!empty()) {
            erase(begin());
        }
    }

    // TODO splice(pos, list&), splice(pos, list&, it), splice(pos, list&, first, last)

    constexpr unsigned remove(T const& t) {
        return remove_if([&](T const& x) { return x == t;});
    }

    template <class UP> constexpr unsigned remove_if(UP p) {
        link l = head;
        unsigned rslt = 0;
        while (l != nil) {
            link n = fwd(l);
            if (p(val(l))) {
                erase(l);
                ++rslt;
            }
            l = n;
        }
        return rslt;
    }

    constexpr unsigned unique() {
        return unique([](T const& t, T const& u) { return t == u;});
    }

    template <class BP> constexpr unsigned unique(BP p) {
        link f = head;
        link l = fwd(head);
        unsigned rslt = 0;
        while (l != nil) {
            link n = fwd(l);
            if (p(val(l), val(f))) {
                erase(l);
                ++rslt;
            }
            else {
                f = l;
            }
            l = n;
        }
        return rslt;
    }

    constexpr void reverse() {
        link l = head;
        link t = tail;
        while (l != t) {
            std::swap(val(l), val(t));
            l = fwd(l);
            if (l == t) {
                break;
            }
            t = bck(t);
        }
    }

    constexpr void sort() {
        sort([](T const& t, T const& u) { return t < u;});
    }

    template <class C> constexpr void sort(C comp) {
        if (size() < 2) { return; }
        head = msort(head, size(), comp);
        link p = nil;                       // the forward links are right, redo the rest
        for (link l = head; l != nil; l = fwd(l)) {
            index i;
            if (i.be(l.get())) { prev.set(i, p); }
            p = l;
        }
        tail = p;
    }

    constexpr link lbegin() const { return head; }
    constexpr link lend() const { return nil; }
    constexpr link lfront() const { return head; }
    constexpr link lback() const { return tail; }

    constexpr link fwd(link l) const {
        index lidx;
        if (lidx.be(l.get())) {
            return next.get(lidx);
        }
        else {
            return nil;
        }
    }
    constexpr link bck(link l) const {
        index lidx;
        if (lidx.be(l.get())) {
            return prev.get(lidx);
        }
        else {
            return nil;
        }
    }
    // Is there an element at `l`? O(1).
    constexpr bool live(link l) const {
        index i;
        return i.be(l.get()) && (prev.get(i) != l);
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

    coblist(coblist const& x) : head(x.head), tail(x.tail), next(x.next), prev(x.prev), vacant(x.vacant), count(x.count) {
        copy_elements(x);
    }
    coblist& operator=(coblist const& x) {
        if (this != &x) {
            clear();
            head = x.head; tail = x.tail; next = x.next; prev = x.prev; vacant = x.vacant; count = x.count;
            copy_elements(x);
        }
        return *this;
    }
    ~coblist() { clear(); }

private:
    static coblist& nowhere() { static thread_local coblist none{}; return none; }

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

    constexpr void copy_elements(coblist const& x) {
        for (link l = head; l != nil; l = fwd(l)) {
            index i;
            if (i.be(l.get())) { store.make(i, x.store.at(i)); }
        }
    }

    cobstore<T, N> store;
    link head;
    link tail;
    cobarray<link, N> next;
    cobarray<link, N> prev;
    link vacant;
    unsigned count = 0;
};


#endif // define      INC_COBLIST
