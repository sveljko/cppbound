/* Compile time bound vector. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBVECTOR)
#define      INC_COBVECTOR


#include "cobbound.hpp"

#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>


// An array that grows, up to its capacity, and shrinks. The elements are
// constructed when put in and destroyed when taken out.
//
// An `index` is known, at compile time, to be within the capacity, so no
// access is ever out of the storage. Whether there is an element at the
// index right now depends on the size, which is only known at run time;
// thus the access by index says if there was an element (like a stale link
// of a list, an index past the size is harmless). Access that needs no
// such check: `for_each()`, and `get()` in a loop over `irange()`.
//
// The iterators are for the algorithms. Like those of the array, they can't
// be moved out of the vector (what would get out ends up at the end) and,
// where there is no element (any more), they refer to a "safety spot".
template <class T, class B>
struct bndvector {
    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;
    using size_type       = typename B::size_type;
    using index           = typename B::index;
    using position   = typename B::link;    // an index, or the one past the last one

    bndvector() = default;
    bndvector(bndvector const& x)
    {
        for (size_type i = 0; i < x.count; ++i) {
            emplace_back(x.ref(i));
        }
    }
    bndvector& operator=(bndvector const& x)
    {
        if (this != &x) {
            clear();
            for (size_type i = 0; i < x.count; ++i) {
                emplace_back(x.ref(i));
            }
        }
        return *this;
    }
    ~bndvector() { clear(); }

    size_type size() const noexcept { return count; }
    size_type max_size() const noexcept { return B::size(); }
    size_type capacity() const noexcept { return B::size(); }
    bool      empty() const noexcept { return count == 0; }
    bool      full() const noexcept { return count == B::size(); }

    // false if there is no room
    template <class... A> bool emplace_back(A&&... a)
    {
        slot s;
        if (!s.be(count)) {
            return false;
        }
        store.make(s, std::forward<A>(a)...);
        ++count;
        return true;
    }
    bool push_back(T const& t) { return emplace_back(t); }
    bool push_back(T&& t) { return emplace_back(std::move(t)); }

    template <template <class> class V> V<T> pop_back()
    {
        if (count == 0) {
            return {};
        }
        V<T> rslt(std::move(ref(count - 1)));
        drop_back();
        return rslt;
    }

    // Before the element at `pos`, which may be the one past the last one.
    // false if there is no room, or `pos` is past the size.
    template <class... A> bool emplace(position pos, A&&... a)
    {
        const size_type at = static_cast<size_type>(pos.get());
        if ((at > count) || full()) {
            return false;
        }
        T fresh(std::forward<A>(a)...);     // first, as `a` may refer to an element
        if (at == count) {
            return emplace_back(std::move(fresh));
        }
        emplace_back(std::move(ref(count - 1)));
        for (size_type i = count - 2; i > at; --i) {
            ref(i) = std::move(ref(i - 1));
        }
        ref(at) = std::move(fresh);
        return true;
    }
    bool insert(position pos, T const& t) { return emplace(pos, t); }
    bool insert(position pos, T&& t) { return emplace(pos, std::move(t)); }

    // false if there is no element at `pos`
    bool erase(index pos) { return erase_at(static_cast<size_type>(pos.get())); }

    // A copy of the element, or nothing
    template <template <class> class V> V<T> at(index i) const
    {
        return live(i) ? V<T>(ref(static_cast<size_type>(i.get()))) : V<T>{};
    }
    template <template <class> class V> V<T> front() const
    {
        return (count == 0) ? V<T>{} : V<T>(ref(0));
    }
    template <template <class> class V> V<T> back() const
    {
        return (count == 0) ? V<T>{} : V<T>(ref(count - 1));
    }
    // A copy of the element, or `T{}` if there is none
    T get(index i) const { return live(i) ? ref(static_cast<size_type>(i.get())) : T{}; }

    // false if there is no element at `i`
    bool set(index i, T const& t)
    {
        return with(i, [&](T& x) { x = t; });
    }
    bool set(index i, T&& t)
    {
        return with(i, [&](T& x) { x = std::move(t); });
    }
    // Calls f with the element at i, if there is one. Returns whether it did.
    template <class F> bool with(index i, F&& f)
    {
        if (live(i)) {
            f(ref(static_cast<size_type>(i.get())));
            return true;
        }
        return false;
    }
    template <class F> bool with(index i, F&& f) const
    {
        if (live(i)) {
            f(ref(static_cast<size_type>(i.get())));
            return true;
        }
        return false;
    }
    // Is there an element at `i`?
    bool live(index i) const { return static_cast<size_type>(i.get()) < count; }

    // false if there is no element at one of them
    bool swap(index i, index j)
    {
        if (!live(i) || !live(j)) {
            return false;
        }
        using std::swap;
        swap(ref(static_cast<size_type>(i.get())), ref(static_cast<size_type>(j.get())));
        return true;
    }
    void swap(bndvector& x)
    {
        bndvector t = *this;
        *this       = x;
        x           = t;
    }

    // Calls f(T&) for each element, first to last
    template <class F> void for_each(F&& f)
    {
        for (size_type i = 0; i < count; ++i) {
            f(ref(i));
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (size_type i = 0; i < count; ++i) {
            f(ref(i));
        }
    }

    // The iterators differ in constness only: `C` is the vector, `U` the
    // element. An iterator keeps the number of its element, at most the size
    // (as it is when the iterator is moved).
    template <class C, class U> class iter {
        C*        r;
        size_type at;

        // Where moving by `x` gets to: the end, if that would be out of the vector
        size_type moved(difference_type x) const
        {
            const size_type n = r->count;
            if (at > n) {
                return n;
            }
            if (x >= 0) {
                return (static_cast<std::size_t>(x) <= n - at) ? at + static_cast<size_type>(x) : n;
            }
            const std::size_t back = static_cast<std::size_t>(-(x + 1)) + 1;
            return (back <= at) ? at - static_cast<size_type>(back) : n;
        }

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = U*;
        using reference         = U&;
        using iterator_category = std::random_access_iterator_tag;

        iter() : r(&nowhere()), at(0) {}
        // A constant iterator from one that is not
        template <class C2, class U2>
            requires(std::is_const_v<C> && !std::is_const_v<C2>)
        iter(iter<C2, U2> const& x) : r(x.r), at(x.at) {}

        iter& operator++() { return *this = *this + 1; }
        iter  operator++(int) { iter t = *this; ++*this; return t; }
        iter& operator--() { return *this = *this - 1; }
        iter  operator--(int) { iter t = *this; --*this; return t; }
        iter  operator+(difference_type x) const { return { r, moved(x) }; }
        iter& operator+=(difference_type x) { return *this = *this + x; }
        iter  operator-(difference_type x) const
        {
            // the least number has no opposite
            return (x == std::numeric_limits<difference_type>::min()) ? iter{ r, r->count } : iter{ r, moved(-x) };
        }
        iter&           operator-=(difference_type x) { return *this = *this - x; }
        difference_type operator-(iter const& x) const
        {
            return (r == x.r) ? static_cast<difference_type>(at) - static_cast<difference_type>(x.at) : 0;
        }
        reference   operator[](difference_type n) const { return *(*this + n); }
        friend iter operator+(difference_type n, iter const& i) { return i + n; }

        U& operator*() const { return (at < r->count) ? r->ref(at) : spot(); }
        U* operator->() const { return &**this; }

        bool operator==(iter const& x) const { return (r == x.r) && (at == x.at); }
        bool operator<(iter const& x) const { return (r == x.r) && (at < x.at); }
        bool operator<=(iter const& x) const { return (r == x.r) && (at <= x.at); }
        bool operator>(iter const& x) const { return (r == x.r) && (at > x.at); }
        bool operator>=(iter const& x) const { return (r == x.r) && (at >= x.at); }

        friend struct bndvector;
        template <class, class> friend class iter;

    private:
        iter(C* r_, size_type at_) : r(r_), at(at_) {}
    };
    using I              = iter<bndvector, T>;
    using CI             = iter<bndvector const, T const>;
    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, 0 }; }
    I  end() { return I{ this, count }; }
    CI begin() const { return CI{ this, 0 }; }
    CI end() const { return CI{ this, count }; }
    CI cbegin() const { return CI{ this, 0 }; }
    CI cend() const { return CI{ this, count }; }

    // Before the element at `pos`. Returns the iterator to the new element,
    // `end()` if it was not put in (no room, or `pos` is not of this vector).
    I insert(CI pos, T const& t) { return put(pos, t); }
    I insert(CI pos, T&& t) { return put(pos, std::move(t)); }
    // Returns the iterator to what followed the erased element, `end()` if
    // there is none or nothing was erased
    I erase(CI pos)
    {
        if ((pos.r != this) || !erase_at(pos.at)) {
            return end();
        }
        return I{ this, pos.at };
    }
    // From `first` up to, not including, `last`. Returns the iterator to what
    // followed them; `end()` also if that is not a range of this vector (in
    // which case nothing is erased).
    I erase(CI first, CI last)
    {
        if ((first.r != this) || (last.r != this) || (first.at > last.at) || (last.at > count)) {
            return end();
        }
        const size_type n = last.at - first.at;
        for (size_type i = last.at; i < count; ++i) {
            ref(i - n) = std::move(ref(i));
        }
        for (size_type i = 0; i < n; ++i) {
            drop_back();
        }
        return I{ this, first.at };
    }

    // The indices of the elements there are right now, first to last:
    //     for (auto i : v.irange()) { use(v.get(i)); }
    class live_range {
        using walk = decltype(B::indices().begin());
        walk      first;
        size_type n;

    public:
        class iterator {
            walk      at;
            size_type left;

        public:
            using value_type        = index;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = index;
            using iterator_category = std::input_iterator_tag;

            iterator(walk at_, size_type left_) : at(at_), left(left_) {}

            iterator& operator++()
            {
                ++at;
                --left;
                return *this;
            }
            iterator operator++(int) { iterator t = *this; ++*this; return t; }
            index    operator*() const { return *at; }
            bool     operator==(iterator const& x) const { return left == x.left; }
        };

        live_range(size_type n_) : first(B::indices().begin()), n(n_) {}
        iterator begin() const { return { first, n }; }
        iterator end() const { return { first, 0 }; }
    };
    live_range irange() const { return live_range{ count }; }

    // false if there is no room for that many; otherwise, elements are
    // dropped from the end, or copies of `v` are added to it
    bool resize(size_type n, T const& v = T{})
    {
        if (n > B::size()) {
            return false;
        }
        while (count > n) {
            drop_back();
        }
        while (count < n) {
            emplace_back(v);
        }
        return true;
    }

    void clear()
    {
        while (count > 0) {
            drop_back();
        }
    }

    bool operator==(bndvector const& x) const
    {
        if (count != x.count) {
            return false;
        }
        for (size_type i = 0; i < count; ++i) {
            if (!(ref(i) == x.ref(i))) {
                return false;
            }
        }
        return true;
    }

protected:
    // For what is built on the vector (the heap): the element at `i`, which
    // must be less than the size
    T& ref(size_type i)
    {
        slot s;
        s.be(i);
        return store.at(s);
    }
    T const& ref(size_type i) const
    {
        slot s;
        s.be(i);
        return store.at(s);
    }

private:
    using slot = typename B::slot;

    // Where iterators that point to no element end up. One per thread.
    static T& spot() { static thread_local T s{}; return s; }

    static bndvector& nowhere() { static thread_local bndvector none{}; return none; }

    bool erase_at(size_type at)
    {
        if (at >= count) {
            return false;
        }
        for (size_type i = at; i + 1 < count; ++i) {
            ref(i) = std::move(ref(i + 1));
        }
        drop_back();
        return true;
    }

    template <class V> I put(CI pos, V&& v)
    {
        position at;
        if ((pos.r != this) || !at.be(pos.at) || !emplace(at, std::forward<V>(v))) {
            return end();
        }
        return I{ this, pos.at };
    }

    void drop_back()
    {
        slot s;
        if ((count > 0) && s.be(count - 1)) {
            store.drop(s);
            --count;
        }
    }

    // The first `count` slots hold an element each
    typename B::template store<T> store;
    size_type                     count = 0;
};

template <class T, int N> using cobvector = bndvector<T, cobbound<N>>;


#endif // !defined(INC_COBVECTOR)
