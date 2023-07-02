/* Compile time bound doubly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBLIST)
#define      INC_COBLIST


#include "cobarray.hpp"


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
        prev.fill(nil);
    }

    constexpr bool empty() const noexcept { return head == nil; }
    constexpr unsigned size() const noexcept {
        auto l = head;
        unsigned c = 0;
        while (l != nil) {
            ++c;
            l = fwd(l);
        }
        return c;
    }
    constexpr unsigned max_size() const noexcept { return N; }

    constexpr bool push_front(T const& t) {
        if (nil == vacant) {
            return false;
        }
        index vacant_idx;
        vacant_idx.be(vacant.get());
        const link l = vacant;
        vacant = next.get(vacant_idx);
    
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

        d[vacant_idx.get()] = t;

        return true;
    }
    // TODO emplace_front(Args&&...)

    template<template<class> class V> constexpr V<T> pop_front() {
        if (head == nil) {
            return {};
        }
        index head_idx;
        head_idx.be(head.get());
        const auto l = head;
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
        vacant = l;

        return d[head_idx.get()];
    }

    constexpr bool push_back(T const& t) {
        if (nil == vacant) {
            return false;
        }
        index vacant_idx;
        vacant_idx.be(vacant.get());
        const link l = vacant;
        vacant = next.get(vacant_idx);
    
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

        d[vacant_idx.get()] = t;

        return true;
    }
    // TODO emplace_back(Args&&...)

    template<template<class> class V> constexpr V<T> pop_back() {
        if (tail == nil) {
            return {};
        }
        index tail_idx;
        tail_idx.be(tail.get());
        const auto l = tail;
        tail = prev.get(tail_idx);
        if (tail != nil) {
            index new_tail_idx;
            new_tail_idx.be(tail.get());
            next.set(new_tail_idx, nil);
        }
        else {
            head = nil;
        }

        prev.set(tail_idx, vacant);
        vacant = l;

        return d[tail_idx.get()];
    }

    class I {
        coblist * r;
        link l;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;
       
        I(I const& x) : r(x.r), l(x.l) {}

        I& operator=(I& x) {
            r = x.r;
            l = x.l;
            return *this;
        }
        I& operator=(I&& x) {
            r = x.r;
            l = x.l;
            return *this;
        }
        I& operator++() {
            if (l != nil) {
                index i;
                i.be(l.get());
                l = r->next.get(i);
            }
            return *this;
        }
        I& operator--() {
            if (l != nil) {
                index i;
                i.be(l);
                l = r->prev.get(i);
            }
            return *this;
        }

        T& operator*() { return r->d[l.get()]; }
        T const& operator*() const { return r->d[l.get()]; }
        T* operator->() { return r->d + l.get(); }
        T const* operator->() const { return r->ßd + l.get(); }

        bool operator==(I const& x) const { return (r == x.r) && (l == x.l); }
        bool operator!=(I const& x) const { return (r != x.r) || (l != x.l); }

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
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;
       
        CI(CI const& x) : r(x.r), l(x.l) {}
        CI(I const& x) : r(x.r), l(x.l) {}

        CI& operator=(CI& x) {
            r = x.r;
            l = x.l;
            return *this;
        }
        CI& operator=(CI&& x) {
            r = x.r;
            l = x.l;
            return *this;
        }
        CI& operator++() {
            if (l != nil) {
                index i;
                i.be(l.get());
                l = r->next.get(i);
            }
            return *this;
        }
        CI& operator--() {
            if (l != nil) {
                index i;
                i.be(l.get());
                l = r->ßprev.get(i);
            }
            return *this;
        }

        T const& operator*() const { return r->d[l.get()]; }
        T const* operator->() const { return r->d + l.get(); }

        bool operator==(CI const& x) const { return (r == x.r) && (l == x.l); }
        bool operator!=(CI const& x) const { return (r != x.r) || (l != x.l); }

        friend struct coblist;

    protected:
        CI(coblist const* r_, link l_) : r(r_), l(l_) {}
    };

    constexpr I begin() { return I{this, head}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head}; }
    constexpr CI cend() const { return CI{this, nil}; }
    // TODO rbegin(), rend(), crbegin(), crend()

    constexpr link insert(link pos, T const& value) {
        return insert(CI{this, pos}, value).l;
    }
    constexpr I insert(CI pos, T const& value) {
        if (nil == vacant) {
            return I{this, nil};
        }
        if (nil == pos.l) {
            push_back(value);
            return I{this, tail};
        }
        if (pos.l == head) {
            push_front(value);
            return I{this, head};
        }
        index pos_idx;
        pos_idx.be(pos.l.get());
        index vacant_idx;
        vacant_idx.be(vacant.get());
        vacant = next.get(vacant_idx);

        index prev_idx;
        prev_idx.be(bck(pos.l));
        next.set(prev_idx, vacant_idx);
        next.set(vacant_idx, pos.l);
        prev.set(vacant_idx, prev_idx);
        prev.set(pos_idx, vacant_idx);

        d[vacant_idx.get()] = value;

        return I{this, vacant_idx};
    }
    // TODO emplace(pos, Args&&...)
    // TODO swap(coblist&)

    constexpr link erase(link pos) {
        return erase(CI{this, pos}).l;
    }
    constexpr I erase(CI pos) {
        if (nil == pos.l) {
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

        next.set(pos_idx, vacant);
        vacant = pos.l;

        return I{this, next_idx};
    }

    constexpr void clear() {
        while (!empty()) {
            erase(begin());
        }
    }

    // TODO merge(coblist&), merge(coblist&, Compare)
    // TODO splice(pos, list&), splice(pos, list&, it), splice(pos, list&, first, last)

    constexpr unsigned remove(T const& t) {
        return remove_if([&](T const& x) { return x == t;});
    }

    template <class UP> constexpr unsigned remove_if(UP p) {
        link l = head;
        unsigned rslt = 0;
        while (l != nil) {
            link n = fwd(l);
            if (p(d[l.get()])) {
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
        if (head == tail) {
            return 0;
        }
        link f = head;
        link l = fwd(head);
        unsigned rslt = 0;
        while (l != nil) {
            link n = fwd(l);
            if (p(d[l.get()], d[f.get()])) {
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
        if (head == tail) {
            return;
        }
        link l = head;
        link t = tail;
        while (l != t) {
            std::swap(d[l.get()], d[t.get()]);
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
        if (head == tail) {
            return;
        }
        link f = head;
        while (f != nil) {
            link l = fwd(f);
            while (l != nil) {
                if (!comp(d[f.get()], d[l.get()])) {
                    std::swap(d[l.get()], d[f.get()]);
                }
                l = fwd(l);
            }
            f = fwd(f);
        }
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
    constexpr bool contains(link l) {
        link p = head;
        while (p != nil) {
            if (p == l) {
                return true;
            }
            p = fwd(p);
        }
        return false;
    }
    constexpr bool contains(CI pos) {
        return (pos.r == this) && contains(pos.l);
    }

private:
    template <class U> struct ignore {
        ignore() {}
        ignore(U const&) {}
    }
    
    T d[N+1];
    link head;
    link tail;
    cobarray<link, N> next;
    cobarray<link, N> prev;
    link vacant;
};


#endif // define      INC_COBLIST
