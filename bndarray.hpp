/* Bound arrays: what `cobarray` and `atbarray` are. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_BNDARRAY)
#define      INC_BNDARRAY


#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


namespace cobhlp {
// To make what the iterators that refer to no container point to
struct nowhere_t {};
} // namespace cobhlp


// An array of as many elements as the bound `B` says, all of them there for
// as long as the array is. It is indexed by the index of the bound, which is
// known to be within it, so no access is ever out of the array.
// There is one element more than that: the "safety spot", where iterators
// that would get out of the array end up.
template <class T, class B>
struct bndarray {
    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;
    using size_type       = typename B::size_type;

    using index = typename B::index;

    template <class U> struct taken_t {
        constexpr taken_t(U& t) : t_(t) {}

        taken_t(taken_t&&)            = delete;
        taken_t& operator=(taken_t&&) = delete;

        constexpr U* operator->() && { return &t_; }

    private:
        U& t_;
    };
    using taken  = taken_t<T>;
    using ctaken = taken_t<T const>;

    constexpr bndarray() = default;

    // Exactly as many values as there are elements, if that is known at compile time
    template <class... U>
        requires(B::fixed && (sizeof...(U) == B::size()) && (std::is_convertible_v<U, T> && ...))
    constexpr bndarray(U&&... u) : d(std::forward<U>(u)...) {}

    // Every element a copy of v
    static constexpr bndarray filled(T const& v)
    {
        bndarray r;
        r.fill(v);
        return r;
    }

    // Element at index i is f(i)
    template <class F> static constexpr bndarray generate(F f)
    {
        bndarray r;
        for (auto i : irange()) {
            r.at(i) = f(i);
        }
        return r;
    }

    constexpr bool      empty() const noexcept { return B::size() == 0; }
    constexpr size_type size() const noexcept { return B::size(); }
    constexpr size_type max_size() const noexcept { return B::size(); }

    constexpr ctaken grab(index i) const { return at(i); }
    constexpr taken  grab(index i) { return at(i); }
    constexpr T      get(index i) const { return at(i); }
    constexpr T      set(index i, T const& t) { return at(i) = t; }
    constexpr T      set(index i, T&& t) { return at(i) = std::move(t); }

    // false if there is no such element
    template <class U> constexpr bool maybe_set(U i, T const& t)
    {
        if (std::cmp_less(i, 0) || !std::cmp_less(i, B::size())) {
            return false;
        }
        d.data()[static_cast<std::size_t>(i)] = t;
        return true;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> get(index i, V U::*m) const
    {
        return at(i).*m;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> set(index i, V U::*m, V const& v)
    {
        return at(i).*m = v;
    }

    constexpr void fill(T const& v)
    {
        for (std::ptrdiff_t i = 0; i < total(); ++i) {
            d.data()[i] = v;
        }
    }

    constexpr bool operator==(bndarray const& x) const
    {
        return std::equal(d.data(), d.data() + total(), x.d.data());
    }

    constexpr T front() const { return d.data()[0]; }
    constexpr T back() const { return d.data()[(total() > 0) ? total() - 1 : 0]; }

    constexpr void swap(bndarray& x)
    {
        for (std::ptrdiff_t i = 0; i < total(); ++i) {
            std::swap(d.data()[i], x.d.data()[i]);
        }
    }

    // The iterators differ in constness only: `A` is the array, `U` the
    // element. An iterator keeps the offset from the first element, which is
    // at most the size; at the size, there is the safety spot.
    template <class A, class U> class iter {
        A*             r;
        std::ptrdiff_t at;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = U*;
        using reference         = U&;
        using iterator_category = std::random_access_iterator_tag;

        iter() : r(&nowhere()), at(r->total()) {}
        // A constant iterator from one that is not
        template <class A2, class U2>
            requires(std::is_const_v<A> && !std::is_const_v<A2>)
        iter(iter<A2, U2> const& x) : r(x.r), at(x.at) {}

        iter& operator++()
        {
            if (at < r->total()) {
                ++at;
            }
            return *this;
        }
        iter  operator++(int) { iter t = *this; ++*this; return t; }
        iter& operator--()
        {
            if (at > 0) {
                --at;
            }
            return *this;
        }
        iter            operator--(int) { iter t = *this; --*this; return t; }
        iter            operator+(difference_type x) const { return { r, forth(at, x, r->total()) }; }
        iter&           operator+=(difference_type x) { return *this = (*this) + x; }
        iter            operator-(difference_type x) const { return { r, back(at, x, r->total()) }; }
        iter&           operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(iter const& x) const { return (r == x.r) ? at - x.at : 0; }
        reference       operator[](difference_type n) const { return *(*this + n); }
        friend iter     operator+(difference_type n, iter const& i) { return i + n; }

        U& operator*() const { return r->d.data()[at]; }
        U* operator->() const { return &**this; }

        bool operator==(iter const& x) const { return (r == x.r) && (at == x.at); }
        bool operator<(iter const& x) const { return (r == x.r) && (at < x.at); }
        bool operator<=(iter const& x) const { return (r == x.r) && (at <= x.at); }
        bool operator>(iter const& x) const { return (r == x.r) && (at > x.at); }
        bool operator>=(iter const& x) const { return (r == x.r) && (at >= x.at); }

        friend struct bndarray;
        template <class, class> friend class iter;

    private:
        iter(A* r_, difference_type at_) : r(r_), at(at_) {}
    };
    using I              = iter<bndarray, T>;
    using CI             = iter<bndarray const, T const>;
    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, 0 }; }
    I  end() { return I{ this, total() }; }
    CI begin() const { return CI{ this, 0 }; }
    CI end() const { return CI{ this, total() }; }
    CI cbegin() const { return CI{ this, 0 }; }
    CI cend() const { return CI{ this, total() }; }

    // for (auto i : a.irange())
    static constexpr auto irange() { return B::indices(); }

private:
    // What the iterators that refer to no array point to. Making it does not
    // look at the size of the bound (which, if that is yet to be set, would
    // make it final), and neither do the iterators: they go by the size of
    // the array they are of.
    explicit bndarray(cobhlp::nowhere_t at) : d(at) {}
    static bndarray& nowhere() { static thread_local bndarray none{ cobhlp::nowhere_t{} }; return none; }

    constexpr std::ptrdiff_t total() const { return static_cast<std::ptrdiff_t>(d.count()); }

    constexpr T&       at(index i) { return d.data()[i.get()]; }
    constexpr T const& at(index i) const { return d.data()[i.get()]; }

    // Moving an iterator at offset `at` (in [0, size]) by `x`. All that would
    // end up out of bounds lands on the safety spot (offset size).
    static constexpr std::ptrdiff_t forth(std::ptrdiff_t at, std::ptrdiff_t x, std::ptrdiff_t n)
    {
        if (x >= 0) {
            return (x <= n - at) ? at + x : n;
        }
        return (x >= -at) ? at + x : n;
    }
    static constexpr std::ptrdiff_t back(std::ptrdiff_t at, std::ptrdiff_t x, std::ptrdiff_t n)
    {
        if (x >= 0) {
            return (x <= at) ? at - x : n;
        }
        return (x >= at - n) ? at - x : n;
    }

    // One element more than the size
    typename B::template buffer<T> d;
};


#endif // !defined(INC_BNDARRAY)
