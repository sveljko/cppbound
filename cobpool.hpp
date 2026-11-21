/* Compile time bound object pool. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBPOOL)
#define      INC_COBPOOL


#include "cobbound.hpp"

#include <bit>
#include <cstdint>
#include <type_traits>
#include <utility>


namespace cobhlp {

// Which slots hold an object: a byte for each slot. The quickest to ask and
// to change one; to find the next one, all the slots up to it are looked at.
template <class B> struct livebytes {
    using slot      = typename B::slot;
    using size_type = typename B::size_type;

    livebytes() { a.fill(false); }

    bool get(slot i) const { return a.get(i); }
    void set(slot i, bool v) { a.set(i, v); }
    // The number of the first slot, from `start` on, that holds an object;
    // the number of slots if there is none
    size_type from(size_type start) const
    {
        for (slot i; (start < B::size()) && i.be(start); ++start) {
            if (a.get(i)) {
                return start;
            }
        }
        return B::size();
    }

private:
    typename B::template array<bool> a;
};

// The same, with a bit for each slot: an eighth of the room, and the next
// one is found 64 slots at a time
template <class B> struct livebits {
    using slot      = typename B::slot;
    using size_type = typename B::size_type;

    bool get(slot i) const { return ((w[word(i)] >> (at(i) % 64)) & 1u) != 0; }
    void set(slot i, bool v)
    {
        const std::uint64_t bit = std::uint64_t{ 1 } << (at(i) % 64);
        if (v) {
            w[word(i)] |= bit;
        }
        else {
            w[word(i)] &= ~bit;
        }
    }
    size_type from(size_type start) const
    {
        if (start >= B::size()) {
            return B::size();
        }
        for (unsigned k = static_cast<unsigned>(start / 64); k < w.count(); ++k) {
            std::uint64_t x = w[k];
            if (k == start / 64) {
                x &= ~std::uint64_t{ 0 } << (start % 64);
            }
            if (x != 0) {
                return static_cast<size_type>(size_type{ k } * 64 + static_cast<unsigned>(std::countr_zero(x)));
            }
        }
        return B::size();
    }

private:
    static size_type at(slot i) { return static_cast<size_type>(i.get()); }
    static unsigned  word(slot i) { return static_cast<unsigned>(at(i) / 64); }

    typename B::words w;    // the bits of no slot are never set
};

} // namespace cobhlp


// A fixed number of slots; objects are made and dropped one by one and are
// referred to by a handle. A handle of a dropped object is harmless (but if
// a new object took the slot, it refers to that one).
// The containers that keep their elements in slots that come and go (lists,
// skiplists, timers) are built on this, so that it's done in one place only.
//
// With `CHECKED` being `true`, a handle also tells which of the objects that
// were ever in its slot it is of, so the handle of a dropped object stays
// harmless when a new object takes the slot. See below.
//
// With `HIVE` being `true`, the pool is also good at going over its objects:
// which slots hold one is kept as a bit each, so those that hold none are
// skipped 64 at a time (and that takes an eighth of the room), and there is
// `first()` and `next()` to go from object to object by their handles. It
// costs a little more to ask if a slot holds an object.
template <class T, class B, bool CHECKED = false, bool HIVE = false>
struct bndpool {
    using value_type = T;
    using size_type  = typename B::size_type;
    using handle     = typename B::link;
    using index      = typename B::slot;

    static constexpr auto nil = B::nil;

    bndpool() : vacant()
    {
        for (auto i : B::slots()) {
            handle n;
            n.be(i.get() + 1);
            links.set(i, n);
        }
    }
    bndpool(bndpool const& x) : links(x.links), used(x.used), vacant(x.vacant), count(x.count)
    {
        copy_objects(x);
    }
    bndpool& operator=(bndpool const& x)
    {
        if (this != &x) {
            clear();
            links  = x.links;
            used   = x.used;
            vacant = x.vacant;
            count  = x.count;
            copy_objects(x);
        }
        return *this;
    }
    ~bndpool() { clear(); }

    // `nil` if there is no room
    template <class... A> handle make(A&&... a)
    {
        index i;
        if (!i.be(vacant.get())) {
            return nil;
        }
        const handle h = vacant;
        vacant         = links.get(i);
        links.set(i, nil);
        used.set(i, true);
        ++count;
        store.make(i, std::forward<A>(a)...);
        return h;
    }
    // false if there is no object at `h`
    bool drop(handle h)
    {
        index i;
        if (!i.be(h.get()) || !used.get(i)) {
            return false;
        }
        store.drop(i);
        used.set(i, false);
        links.set(i, vacant);
        vacant = h;
        --count;
        return true;
    }
    // Is there an object at `h`? O(1).
    bool live(handle h) const
    {
        index i;
        return i.be(h.get()) && used.get(i);
    }

    // Calls f with the object at h, if there is one. Returns whether it did.
    template <class F> bool with(handle h, F&& f)
    {
        index i;
        if (i.be(h.get()) && used.get(i)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    template <class F> bool with(handle h, F&& f) const
    {
        index i;
        if (i.be(h.get()) && used.get(i)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    // A copy, or `T{}` if there is no object
    T get(handle h) const
    {
        index i;
        return (i.be(h.get()) && used.get(i)) ? store.at(i) : T{};
    }

    // In index order
    template <class F> void for_each(F&& f)
    {
        index i;
        for (size_type s = used.from(0); i.be(s); s = used.from(s + 1)) {
            f(store.at(i));
        }
    }
    template <class F> void for_each(F&& f) const
    {
        index i;
        for (size_type s = used.from(0); i.be(s); s = used.from(s + 1)) {
            f(store.at(i));
        }
    }

    // The handle of the first object, in index order, and of the one after
    // that of `h`; `nil` if there is none. Going over all the objects:
    //     for (auto h = p.first(); h != p.nil; h = p.next(h))
    handle first() const
        requires HIVE
    {
        return first_node();
    }
    handle next(handle h) const
        requires HIVE
    {
        return next_node(h);
    }

    size_type size() const noexcept { return count; }
    size_type max_size() const noexcept { return B::size(); }
    bool      empty() const noexcept { return count == 0; }
    bool      full() const noexcept { return count == B::size(); }

    void clear()
    {
        index i;
        for (size_type s = used.from(0); i.be(s); s = used.from(s + 1)) {
            drop(i);
        }
    }

protected:
    // For the containers built on a pool, which know what is live. To them,
    // an object is at a `node`: its slot, or `nil`. Each slot has a link:
    // the pool chains the vacant slots with it, and that of a live slot is
    // its owner's to use (it is `nil` in a new object).
    using node = typename B::link;

    T&       at(index i) { return store.at(i); }
    T const& at(index i) const { return store.at(i); }
    node     link_of(index i) const { return links.get(i); }
    void     link_to(index i, node l) { links.set(i, l); }

    template <class... A> node make_node(A&&... a) { return make(std::forward<A>(a)...); }
    bool drop_node(node n) { return drop(n); }
    bool live_node(node n) const { return live(n); }
    // What is given out for a node, and the node of what was given out. In
    // the pool with checked handles, these two are where the checking is.
    handle ticket(node n) const { return n; }
    node   raw(handle h) const { return h; }

    // The first node that holds an object, and the first one after `n`
    node first_node() const
    {
        node n;
        n.be(used.from(0));
        return n;
    }
    node next_node(node n) const
    {
        index i;
        if (!i.be(n.get())) {
            return nil;
        }
        n.be(used.from(static_cast<size_type>(i.get()) + 1));
        return n;
    }

private:
    void copy_objects(bndpool const& x)
    {
        index i;
        for (size_type s = used.from(0); i.be(s); s = used.from(s + 1)) {
            store.make(i, x.store.at(i));
        }
    }

    typename B::template store<T>      store;
    typename B::template array<handle> links;
    std::conditional_t<HIVE, cobhlp::livebits<B>, cobhlp::livebytes<B>> used;
    handle                             vacant;
    size_type                          count = 0;
};



// The pool with checked handles. Each slot counts the objects dropped from
// it (its "generation"), and a handle is of the generation it was made in.
// That takes a counter per slot and a comparison per access. The counter is
// an `unsigned`: only after a slot was reused as many times as that can
// count, a handle that old could be taken for a new one.
template <class T, class B, bool HIVE>
struct bndpool<T, B, true, HIVE> : protected bndpool<T, B, false, HIVE> {
    using plain      = bndpool<T, B, false, HIVE>;
    using value_type = T;
    using size_type  = typename B::size_type;
    using index      = typename B::slot;

    static constexpr auto nil = B::nil;

    // Of no object, unless it was got from `make()`
    class handle {
        typename B::link at;
        unsigned         generation = 0;

        handle(typename B::link at_, unsigned generation_) : at(at_), generation(generation_) {}
        friend struct bndpool;

    public:
        handle() : at(B::nil) {}
        handle(decltype(B::nil)) : at(B::nil) {}

        friend bool operator==(handle const& x, handle const& y)
        {
            return (x.at == y.at) && (x.generation == y.generation);
        }
    };

    using plain::empty;
    using plain::for_each;
    using plain::full;
    using plain::max_size;
    using plain::size;

    // `nil` if there is no room
    template <class... A> handle make(A&&... a) { return ticket(make_node(std::forward<A>(a)...)); }
    // false if the object of `h` is not there (any more)
    bool drop(handle h) { return drop_node(raw(h)); }
    // Is the object of `h` there? O(1).
    bool live(handle h) const { return raw(h) != nil; }

    // Calls f with the object of h, if it is there. Returns whether it did.
    template <class F> bool with(handle h, F&& f) { return plain::with(raw(h), std::forward<F>(f)); }
    template <class F> bool with(handle h, F&& f) const { return plain::with(raw(h), std::forward<F>(f)); }
    // A copy, or `T{}` if the object is not there
    T get(handle h) const { return plain::get(raw(h)); }

    // `nil` also if the object of `h` is not there any more
    handle first() const
        requires HIVE
    {
        return ticket(plain::first_node());
    }
    handle next(handle h) const
        requires HIVE
    {
        return ticket(plain::next_node(raw(h)));
    }

    void clear()
    {
        for (node n = plain::first_node(); n != nil; n = plain::next_node(n)) {
            drop_node(n);
        }
    }

protected:
    // For the containers built on the pool: see the pool that is not checked
    using typename plain::node;
    using plain::at;
    using plain::link_of;
    using plain::link_to;

    template <class... A> node make_node(A&&... a) { return plain::make(std::forward<A>(a)...); }
    // Whatever was given out for the node is good no more
    bool drop_node(node n)
    {
        index i;
        if (!i.be(n.get()) || !plain::live(n)) {
            return false;
        }
        generations.set(i, generations.get(i) + 1u);
        return plain::drop(n);
    }
    bool live_node(node n) const { return plain::live(n); }
    // `nil`, if there is no object at the node
    handle ticket(node n) const
    {
        index i;
        return (i.be(n.get()) && plain::live(n)) ? handle{ n, generations.get(i) } : handle{};
    }
    // `nil`, if what the handle was given out for is not there any more
    node raw(handle h) const
    {
        index i;
        return (i.be(h.at.get()) && plain::live(h.at) && (generations.get(i) == h.generation)) ? h.at
                                                                                              : node{ nil };
    }

private:
    typename B::template array<unsigned> generations;
};

template <class T, int N> using cobpool        = bndpool<T, cobbound<N>>;
template <class T, int N> using cobcheckedpool = bndpool<T, cobbound<N>, true>;
// The pool that is good at going over its objects, and the one that also has checked handles
template <class T, int N> using cobhive        = bndpool<T, cobbound<N>, false, true>;
template <class T, int N> using cobcheckedhive = bndpool<T, cobbound<N>, true, true>;


#endif // !defined(INC_COBPOOL)
