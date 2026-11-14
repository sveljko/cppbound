/* Allocation time bound array. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBARRAY)
#define      INC_ATBARRAY


#include "atbdim.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>


// Why this is sound (keep it true): every public constructor freezes the
// dimension and allocates exactly that many elements; an index can only be
// made after the dimension is frozen and is below it; the dimension never
// changes after it is frozen. The only array with a different size is
// `nowhere()`, which can only be reached by iterators, and those clamp with
// the array's own `n`.
template <class T, class Tag> class atbarray {
    std::size_t          n;
    std::unique_ptr<T[]> d;

    explicit atbarray(std::nullptr_t) : n(0), d(new T[1]()) {}

public:
    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;

    using index = atbidx<Tag>;

    atbarray() : n(atbdim<Tag>::get()), d(new T[n + 1]()) {}

    // No move operations: a moved-from array would have no storage, so a move is a copy
    atbarray(atbarray const& x) : n(x.n), d(new T[x.n + 1]())
    {
        std::copy(x.d.get(), x.d.get() + n, d.get());
    }
    atbarray& operator=(atbarray const& x)
    {
        if (this != &x) {
            atbarray t(x);
            swap(t);
        }
        return *this;
    }
    void swap(atbarray& x)
    {
        std::swap(n, x.n);
        std::swap(d, x.d);
    }

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

    bool        empty() const noexcept { return n == 0; }
    std::size_t size() const noexcept { return n; }
    std::size_t max_size() const noexcept { return n; }

    ctaken grab(index i) const { return d[i.get()]; }
    taken  grab(index i) { return d[i.get()]; }
    T      get(index i) const { return d[i.get()]; }
    T      set(index i, T const& t) { return d[i.get()] = t; }
    T      set(index i, T&& t) { return d[i.get()] = std::move(t); }

    bool maybe_set(std::size_t i, T const& t)
    {
        if (i < n) {
            d[i] = t;
            return true;
        }
        return false;
    }
    template <class V, class U = T>
    std::enable_if_t<std::is_class_v<U>, V> get(index i, V U::*m) const
    {
        return d[i.get()].*m;
    }
    template <class V, class U = T>
    std::enable_if_t<std::is_class_v<U>, V> set(index i, V U::*m, V const& v)
    {
        return d[i.get()].*m = v;
    }

    void fill(T const& v)
    {
        for (std::size_t i = 0; i < n; ++i) {
            d[i] = v;
        }
    }

    bool operator==(atbarray const& x) const
    {
        return (n == x.n) && std::equal(d.get(), d.get() + n, x.d.get());
    }

    static atbarray filled(T const& v)
    {
        atbarray r;
        r.fill(v);
        return r;
    }
    // Element at index i is f(i)
    template <class F> static atbarray generate(F f)
    {
        atbarray r;
        for (auto i : irange()) {
            r.d[i.get()] = f(i);
        }
        return r;
    }

    static auto irange() { return index::range(); }

    class CI;
    class I {
        atbarray* r;
        T*        p;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;
        using iterator_category = std::random_access_iterator_tag;

        I() : r(&nowhere()), p(r->d.get() + r->n) {}

        I& operator++()
        {
            if (p - r->d.get() < dim()) {
                ++p;
            }
            return *this;
        }
        I operator++(int)
        {
            I t = *this;
            ++*this;
            return t;
        }
        I& operator--()
        {
            if (p > r->d.get()) {
                --p;
            }
            return *this;
        }
        I operator--(int)
        {
            I t = *this;
            --*this;
            return t;
        }
        I operator+(difference_type x) const
        {
            return { r, r->d.get() + forth(p - r->d.get(), x, dim()) };
        }
        I& operator+=(difference_type x) { return *this = (*this) + x; }
        I operator-(difference_type x) const
        {
            return { r, r->d.get() + back(p - r->d.get(), x, dim()) };
        }
        I& operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(I const& x) const { return (r == x.r) ? p - x.p : 0; }
        reference operator[](difference_type n_) const { return *(*this + n_); }
        friend I operator+(difference_type n_, I const& i) { return i + n_; }

        T& operator*() const { return *p; }
        T* operator->() const { return p; }

        bool operator==(I const& x) const { return (r == x.r) && (p == x.p); }
        bool operator<(I const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(I const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator>(I const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(I const& x) const { return (r == x.r) && (p >= x.p); }

        friend class atbarray;
        friend class CI;

    protected:
        I(atbarray const* r_, T const* p_)
            : r(const_cast<atbarray*>(r_)), p(const_cast<T*>(p_))
        {
        }
        difference_type dim() const { return static_cast<difference_type>(r->n); }
    };
    class CI {
        atbarray const* r;
        T const*        p;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T const*;
        using reference         = T const&;
        using iterator_category = std::random_access_iterator_tag;

        CI() : r(&nowhere()), p(r->d.get() + r->n) {}
        CI(I const& x) : r(x.r), p(x.p) {}

        CI& operator++()
        {
            if (p - r->d.get() < dim()) {
                ++p;
            }
            return *this;
        }
        CI operator++(int)
        {
            CI t = *this;
            ++*this;
            return t;
        }
        CI& operator--()
        {
            if (p > r->d.get()) {
                --p;
            }
            return *this;
        }
        CI operator--(int)
        {
            CI t = *this;
            --*this;
            return t;
        }
        CI operator+(difference_type x) const
        {
            return { r, r->d.get() + forth(p - r->d.get(), x, dim()) };
        }
        CI& operator+=(difference_type x) { return *this = (*this) + x; }
        CI operator-(difference_type x) const
        {
            return { r, r->d.get() + back(p - r->d.get(), x, dim()) };
        }
        CI& operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(CI const& x) const { return (r == x.r) ? p - x.p : 0; }
        reference operator[](difference_type n_) const { return *(*this + n_); }
        friend CI operator+(difference_type n_, CI const& i) { return i + n_; }

        T const& operator*() const { return *p; }
        T const* operator->() const { return p; }

        bool operator==(CI const& x) const { return (r == x.r) && (p == x.p); }
        bool operator<(CI const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(CI const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator>(CI const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(CI const& x) const { return (r == x.r) && (p >= x.p); }

        friend class atbarray;

    protected:
        CI(atbarray const* r_, T const* p_) : r(r_), p(p_) {}
        difference_type dim() const { return static_cast<difference_type>(r->n); }
    };

    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, d.get() }; }
    I  end() { return I{ this, d.get() + n }; }
    CI begin() const { return CI{ this, d.get() }; }
    CI end() const { return CI{ this, d.get() + n }; }
    CI cbegin() const { return CI{ this, d.get() }; }
    CI cend() const { return CI{ this, d.get() + n }; }

private:
    // Must not read the dimension: that would freeze it
    static atbarray& nowhere()
    {
        static thread_local atbarray none{ nullptr };
        return none;
    }

    // Moving an iterator at offset `at` (in [0, dim]) by `x`, without ever
    // forming a pointer out of the array. All that would end up out of
    // bounds lands on the safety spot (offset dim).
    static std::ptrdiff_t forth(std::ptrdiff_t at, std::ptrdiff_t x, std::ptrdiff_t dim)
    {
        if (x >= 0) {
            return (x <= dim - at) ? at + x : dim;
        }
        return (x >= -at) ? at + x : dim;
    }
    static std::ptrdiff_t back(std::ptrdiff_t at, std::ptrdiff_t x, std::ptrdiff_t dim)
    {
        if (x >= 0) {
            return (x <= at) ? at - x : dim;
        }
        return (x >= at - dim) ? at - x : dim;
    }
};


#endif // !defined(INC_ATBARRAY)
