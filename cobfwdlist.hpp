/* Compile time bound singly linked lists. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFWDLIST)
#define      INC_COBFWDLIST


#include "cobarray.hpp"


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
        head = l;

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

        next.set(head_idx, vacant);
        vacant = l;

        return d[head_idx.get()];
    }

    class I {
        cobfwdlist * r;
        link l;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;
       
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
            l = r->fwd(l);
            return *this;
        }

        constexpr T& operator*() { return r->d[l.get()]; }
        constexpr T const& operator*() const { return r->d[l.get()]; }
        constexpr T* operator->() { return r->d + l.get(); }
        constexpr T const* operator->() const { return r->ßd + l.get(); }

        constexpr bool operator==(I const& x) const { return (r == x.r) && (l == x.l); }
        constexpr bool operator!=(I const& x) const { return (r != x.r) || (l != x.l); }

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
            l = r->fwd(l);
            return *this;
        }

        T const& operator*() const { return r->d[l.get()]; }
        T const* operator->() const { return r->d + l.get(); }

        bool operator==(CI const& x) const { return (r == x.r) && (l == x.l); }
        bool operator!=(CI const& x) const { return (r != x.r) || (l != x.l); }

        friend struct cobfwdlist;

    protected:
        CI(cobfwdlist const* r_, link l_) : r(r_), l(l_) {}
    };

    constexpr I begin() { return I{this, head}; }
    constexpr I end() { return I{this, nil}; }
    constexpr CI begin() const { return CI{this, head}; }
    constexpr CI end() const { return CI{this, nil}; }
    constexpr CI cbegin() const { return CI{this, head}; }
    constexpr CI cend() const { return CI{this, nil}; }
    // TODO? before_begin()?

    constexpr link insert_after(link pos, T const& value) {
        return insert_after(CI{this, pos}, value).l;
    }
    constexpr I insert_after(CI pos, T const& value) {
        if (nil == vacant) {
            return I{this, nil};
        }
        index pos_idx;
        pos_idx.be(pos.l.get());
        index vacant_idx;
        vacant_idx.be(vacant.get());
        vacant = next.get(vacant_idx);

        next.set(vacant_idx, next.get(pos_idx));
        next.set(pos_idx, vacant_idx);

        d[vacant_idx.get()] = value;

        return I{this, vacant_idx};
    }
    // TODO emplace_after(pos, Args&&...)
    // TODO swap(coblist&)

    constexpr link erase_after(link pos) {
        return erase_after(CI{this, pos}).l;
    }
    constexpr I erase_after(CI pos) {
        if (nil == pos.l) {
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

        next.set(pos_idx, next.get(next_idx));
        next.set(next_idx, vacant);
        vacant = after;

        return I{this, next.get(pos_idx)};
    }

    constexpr void clear() {
        while (!empty()) {
            pop_front<ignore>();
        }
    }

    // TODO merge(coblist&), merge(coblist&, Compare)
    // TODO splice(pos, list&), splice(pos, list&, it), splice(pos, list&, first, last)

    constexpr unsigned remove(T const& t) {
        return remove_if([&](T const& x) { return x == t;});
    }

    template <class UP> constexpr unsigned remove_if(UP p) {
        unsigned rslt = 0;
        while (!empty() && p(d[head.get()])) {
            pop_front<ignore>();
            ++rslt;
        }
        if (empty()) {
            return rslt;
        }
        link l = head;
        link after = fwd(head);
        while (after != nil) {
            if (p(d[after.get()])) {
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
            if (p(d[after.get()], d[f.get()])) {
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

    constexpr link fwd(link l) const {
        index lidx;
        if (lidx.be(l.get())) {
            return next.get(lidx);
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
    };
    
    T d[N+1];
    link head;
    cobarray<link, N> next;
    link vacant;
};


#endif // define      INC_COBLIST
