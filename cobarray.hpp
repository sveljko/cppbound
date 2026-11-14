/* Compile time bound arrays. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBARRAY)
#define      INC_COBARRAY


#include "cobi.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


template <class T, int N>
struct cobarray {
    static_assert(N > 0, "Array must have at least one element");

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    using index = cobi<int, 0, N-1>;

    template <class U> struct taken_t {
        constexpr taken_t(U& t) : t_(t) {}

        taken_t(taken_t&&) = delete;
        taken_t& operator=(taken_t&&) = delete;

        constexpr U* operator->() && { return &t_; }

    private:
        U& t_;
    };
    using taken = taken_t<T>;
    using ctaken = taken_t<T const>;

    constexpr cobarray() : d{} {}

    // Exactly N values
    template <class... U>
        requires((sizeof...(U) == N) && (std::is_convertible_v<U, T> && ...))
    constexpr cobarray(U&&... u) : d{ T(std::forward<U>(u))..., T{} } {}

    // Every element a copy of v
    static constexpr cobarray filled(T const& v) { cobarray r; r.fill(v); return r; }

    // Element at index i is f(i)
    template <class F> static constexpr cobarray generate(F f)
    {
        cobarray r;
        for (auto i : irange()) { r.d[i.get()] = f(i); }
        return r;
    }

    constexpr bool empty() const noexcept { return false; }
    constexpr unsigned size() const noexcept { return N; }
    constexpr unsigned max_size() const noexcept { return N; }

    constexpr ctaken grab(index i) const { return d[i.get()]; }
    constexpr taken grab(index i) { return d[i.get()]; }
    constexpr T get(index i) const { return d[i.get()]; }
    constexpr T set(index i, T const& t) { return d[i.get()] = t; }
    constexpr T set(index i, T&& t) { return d[i.get()] = std::move(t); }

    constexpr bool maybe_set(int i, T const& t)  {
        index idx;
        if (idx.be(i)) {
            d[idx.get()] = t;
            return true;
        }
        return false;
    }
    template <class V, class U=T>
    constexpr std::enable_if_t<std::is_class_v<U>,V> get(index i, V U::*m) const   {
        return d[i.get()].*m;
    }
    template <class V, class U=T>
    constexpr std::enable_if_t<std::is_class_v<U>,V> set(index i, V U::*m, V const& v) {
        return d[i.get()].*m = v;
    }

    constexpr void fill(T const& v) {
        for (T* p = d; p < d + N; ++p) {
            *p = v;
        }
    }

    constexpr bool operator==(cobarray const& x) const {
        return std::equal(d, d + N, x.d);
    }

    constexpr T front() const { return d[0]; }
    constexpr T back() const { return d[N - 1]; }

    constexpr void swap(cobarray& x) {
        for (int i = 0; i < N; ++i) {
            std::swap(d[i], x.d[i]);
        }
    }

    class CI;
    class I {
        cobarray* r;
        T* p;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        I() : r(&nowhere()), p(r->d + N) {}

        I& operator++() {
            if (p - r->d < N) {
                ++p;
            }
            return *this;
        }
        I operator++(int) { I t = *this; ++*this; return t; }
        I operator--(int) { I t = *this; --*this; return t; }
        reference operator[](difference_type n) const { return *(*this + n); }
        friend I operator+(difference_type n, I const& i) { return i + n; }
        I& operator--() {
            if (p > r->d) {
                --p;
            }
            return *this;
        }
        I operator+(difference_type x) const {
            return { r, r->d + forth(p - r->d, x) };
        }
        I& operator+=(difference_type x) {
            return *this = (*this) + x;
        }
        I operator-(difference_type x) const {
            return { r, r->d + back(p - r->d, x) };
        }
        I& operator-=(difference_type x) {
            return *this = (*this) - x;
        }
        difference_type operator-(I const& x) const {
            return (r == x.r) ? p - x.p : 0;
        }

        T& operator*() const { return *p; }
        T* operator->() const { return p; }

        bool operator==(I const& x) const { return (r == x.r) && (p == x.p); }
        bool operator< (I const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(I const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator> (I const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(I const& x) const { return (r == x.r) && (p >= x.p); }

        friend struct cobarray;
        friend class CI;

    protected:
        I(cobarray const* r_, T const* p_) : r(const_cast<cobarray*>(r_)), p(const_cast<T*>(p_)) {}
    };
    class CI {
        cobarray const* r;
        T const* p;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T const*;
        using reference = T const&;
        using iterator_category = std::random_access_iterator_tag;

        CI() : r(&nowhere()), p(r->d + N) {}
        CI(I const& x) : r(x.r), p(x.p) {}

        CI& operator++() {
            if (p - r->d < N) {
                ++p;
            }
            return *this;
        }
        CI operator++(int) { CI t = *this; ++*this; return t; }
        CI operator--(int) { CI t = *this; --*this; return t; }
        reference operator[](difference_type n) const { return *(*this + n); }
        friend CI operator+(difference_type n, CI const& i) { return i + n; }
        CI& operator--() {
            if (p > r->d) {
                --p;
            }
            return *this;
        }
        CI operator+(difference_type x) const {
            return { r, r->d + forth(p - r->d, x) };
        }
        CI& operator+=(difference_type x) {
            return *this = (*this) + x;
        }
        CI operator-(difference_type x) const {
            return { r, r->d + back(p - r->d, x) };
        }
        CI& operator-=(difference_type x) {
            return *this = (*this) - x;
        }
        difference_type operator-(CI const& x) const {
            return (r == x.r) ? p - x.p : 0;
        }

        T const& operator*() const { return *p; }
        T const* operator->() const { return p; }

        bool operator==(CI const& x) const { return (r == x.r) && (p == x.p); }
        bool operator< (CI const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(CI const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator> (CI const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(CI const& x) const { return (r == x.r) && (p >= x.p); }

        friend struct cobarray;

    protected:
        CI(cobarray const* r_, T const* p_) : r(r_), p(p_) {}
    };

    using iterator = I;
    using const_iterator = CI;

    constexpr I begin() { return I{this, &d[0]}; }
    constexpr I end() { return I{this, d + N}; }
    constexpr CI begin() const { return CI{this, &d[0]}; }
    constexpr CI end() const { return CI{this, d + N}; }
    constexpr CI cbegin() const { return CI{this, &d[0]}; }
    constexpr CI cend() const { return CI{this, d + N}; }

    static constexpr cobirange<int,0,N-1> irange() { return cobirange<int,0,N-1>{}; }

private:
    static cobarray& nowhere() { static thread_local cobarray none{}; return none; }

    // Moving an iterator at offset `at` (in [0, N]) by `x`, without ever
    // forming a pointer out of the array. All that would end up out of
    // bounds lands on the safety spot (offset N).
    static constexpr std::ptrdiff_t forth(std::ptrdiff_t at, std::ptrdiff_t x) {
        if (x >= 0) {
            return (x <= N - at) ? at + x : N;
        }
        return (x >= -at) ? at + x : N;
    }
    static constexpr std::ptrdiff_t back(std::ptrdiff_t at, std::ptrdiff_t x) {
        if (x >= 0) {
            return (x <= at) ? at - x : N;
        }
        return (x >= at - N) ? at - x : N;
    }

    T d[N+1];
};

#endif // define      INC_COBARRAY
