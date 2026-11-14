/* Compile time bound matrix. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBMATRIX)
#define INC_COBMATRIX


#include "cobarray.hpp"
#include "cobi.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>


template <class T, int M, int N> struct cobmatrix {
    static_assert(M > 0, "Matrix must have at least one row");
    static_assert(N > 0, "Matrix must have at least one column");

    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;

    using row    = cobi<int, 0, M - 1>;
    using column = cobi<int, 0, N - 1>;
    struct index {
        row    irow;
        column icolumn;
    };
    template <class U> struct taken_t {
        constexpr taken_t(U& t) : t_(t) {}

        taken_t(taken_t&&) = delete;
        taken_t& operator=(taken_t&&) = delete;

        constexpr U* operator->() && { return &t_; }

    private:
        U& t_;
    };
    using taken  = taken_t<T>;
    using ctaken = taken_t<T const>;

    constexpr bool     empty() const noexcept { return false; }
    constexpr unsigned size() const noexcept { return M * N; }
    constexpr unsigned max_size() const noexcept { return M * N; }

    constexpr ctaken grab(index i) const { return d[i.irow.get()][i.icolumn.get()]; }
    constexpr taken grab(index i) { return d[i.irow.get()][i.icolumn.get()]; }
    constexpr T get(index i) const { return d[i.irow.get()][i.icolumn.get()]; }
    constexpr T set(index i, T const& t)
    {
        return d[i.irow.get()][i.icolumn.get()] = t;
    }
    constexpr T set(index i, T&& t)
    {
        return d[i.irow.get()][i.icolumn.get()] = std::move(t);
    }
    constexpr bool maybe_set(int m, int n, T const& t)
    {
        index idx;
        if (idx.irow.be(m) && idx.icolumn.be(n)) {
            d[m][n] = t;
            return true;
        }
        return false;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> get(index i, V U::*m) const
    {
        return d[i.irow.get()][i.icolumn.get()].*m;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> set(index i, V U::*m, V const& v)
    {
        return d[i.irow.get()][i.icolumn.get()].*m = v;
    }

    // Every element a copy of v
    static constexpr cobmatrix filled(T const& v)
    {
        cobmatrix r;
        for (int m = 0; m < M; ++m) {
            for (int n = 0; n < N; ++n) {
                r.d[m][n] = v;
            }
        }
        return r;
    }

    // Element at index i is f(i)
    template <class F> static constexpr cobmatrix generate(F f)
    {
        cobmatrix r;
        index     i;
        do {
            i.icolumn = column{};
            do {
                r.d[i.irow.get()][i.icolumn.get()] = f(i);
            } while (i.icolumn.advance());
        } while (i.irow.advance());
        return r;
    }

    void fill(T const& v)
    {
        for (auto p = begin(); p != end(); ++p) {
            *p = v;
        }
    }

    constexpr bool operator==(cobmatrix const& x) const
    {
        for (int m = 0; m < M; ++m) {
            if (!std::equal(d[m], d[m] + N, x.d[m])) {
                return false;
            }
        }
        return true;
    }

    constexpr T front() const { return d[0][0]; }
    constexpr T back() const { return d[M - 1][N - 1]; }

    constexpr void swap(cobmatrix& x)
    {
        for (int m = 0; m < M; ++m) {
            for (int n = 0; n < N; ++n) {
                std::swap(d[m][n], x.d[m][n]);
            }
        }
    }

    static constexpr int rows() { return M; }
    static constexpr int columns() { return N; }

    constexpr cobarray<T, N> get_row(row r) const
    {
        cobarray<T, N> rslt;
        for (int n = 0; n < N; ++n) {
            rslt.maybe_set(n, d[r.get()][n]);
        }
        return rslt;
    }
    constexpr void set_row(row r, cobarray<T, N> const& v)
    {
        for (int n = 0; n < N; ++n) {
            column c;
            c.be(n);
            d[r.get()][n] = v.get(c);
        }
    }

    class CI;
    class I {
        cobmatrix* r;
        T (*row)[N];
        T* p;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;
        using iterator_category = std::random_access_iterator_tag;

        I() : I(&nowhere(), M * N) {}
        I operator++(int) { I t = *this; ++*this; return t; }
        I operator--(int) { I t = *this; --*this; return t; }
        reference operator[](difference_type n) const { return *(*this + n); }
        friend I operator+(difference_type n, I const& i) { return i + n; }

        I& operator++()
        {
            if (row - r->d < M) {
                if (++p - *row == N) {
                    ++row;
                    p = *row;
                }
            }
            return *this;
        }
        I& operator--()
        {
            if (p == *row) {
                if (row > r->d) {
                    --row;
                    p = (*row) + (N - 1);
                }
            }
            else {
                --p;
            }
            return *this;
        }
        I operator+(difference_type x) const { return { r, forth(at(), x) }; }
        I& operator+=(difference_type x) { return *this = (*this) + x; }
        I operator-(difference_type x) const { return { r, back(at(), x) }; }
        I& operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(I const& x) const
        {
            return (r == x.r) ? at() - x.at() : 0;
        }

        T& operator*() const { return *p; }
        T* operator->() const { return p; }

        bool operator==(I const& x) const { return (r == x.r) && (p == x.p); }
        bool operator<(I const& x) const { return (r == x.r) && (at() < x.at()); }
        bool operator<=(I const& x) const { return (r == x.r) && (at() <= x.at()); }
        bool operator>(I const& x) const { return (r == x.r) && (at() > x.at()); }
        bool operator>=(I const& x) const { return (r == x.r) && (at() >= x.at()); }

        friend struct cobmatrix;
        friend class CI;

    protected:
        I(cobmatrix const* r_, difference_type at)
            : r(const_cast<cobmatrix*>(r_))
            , row(r->d + at / N)
            , p(*row + at % N)
        {
        }
        // Offset from the first element, in [0, M * N]
        difference_type at() const { return (row - r->d) * N + (p - *row); }
    };
    class CI {
        cobmatrix const* r;
        T const (*row)[N];
        T const* p;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T const*;
        using reference         = T const&;
        using iterator_category = std::random_access_iterator_tag;

        CI() : CI(&nowhere(), M * N) {}
        CI(I const& x) : CI(x.r, x.at()) {}
        CI operator++(int) { CI t = *this; ++*this; return t; }
        CI operator--(int) { CI t = *this; --*this; return t; }
        reference operator[](difference_type n) const { return *(*this + n); }
        friend CI operator+(difference_type n, CI const& i) { return i + n; }

        CI& operator++()
        {
            if (row - r->d < M) {
                if (++p - *row == N) {
                    ++row;
                    p = *row;
                }
            }
            return *this;
        }
        CI& operator--()
        {
            if (p == *row) {
                if (row > r->d) {
                    --row;
                    p = (*row) + (N - 1);
                }
            }
            else {
                --p;
            }
            return *this;
        }
        CI operator+(difference_type x) const { return { r, forth(at(), x) }; }
        CI& operator+=(difference_type x) { return *this = (*this) + x; }
        CI operator-(difference_type x) const { return { r, back(at(), x) }; }
        CI& operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(CI const& x) const
        {
            return (r == x.r) ? at() - x.at() : 0;
        }

        T const& operator*() const { return *p; }
        T const* operator->() const { return p; }

        bool operator==(CI const& x) const { return (r == x.r) && (p == x.p); }
        bool operator<(CI const& x) const { return (r == x.r) && (at() < x.at()); }
        bool operator<=(CI const& x) const { return (r == x.r) && (at() <= x.at()); }
        bool operator>(CI const& x) const { return (r == x.r) && (at() > x.at()); }
        bool operator>=(CI const& x) const { return (r == x.r) && (at() >= x.at()); }

        friend struct cobmatrix;

    protected:
        CI(cobmatrix const* r_, difference_type at)
            : r(r_)
            , row(r->d + at / N)
            , p(*row + at % N)
        {
        }
        // Offset from the first element, in [0, M * N]
        difference_type at() const { return (row - r->d) * N + (p - *row); }
    };

    using iterator = I;
    using const_iterator = CI;

    constexpr I  begin() { return I{ this, 0 }; }
    constexpr I  end() { return I{ this, M * N }; }
    constexpr CI begin() const { return CI{ this, 0 }; }
    constexpr CI end() const { return CI{ this, M * N }; }
    constexpr CI cbegin() const { return CI{ this, 0 }; }
    constexpr CI cend() const { return CI{ this, M * N }; }


private:
    static cobmatrix& nowhere() { static thread_local cobmatrix none{}; return none; }
    // Moving an iterator at offset `at` (in [0, M * N]) by `x`. All that would
    // end up out of bounds lands on the safety spot (offset M * N).
    static constexpr std::ptrdiff_t forth(std::ptrdiff_t at, std::ptrdiff_t x)
    {
        if (x >= 0) {
            return (x <= M * N - at) ? at + x : M * N;
        }
        return (x >= -at) ? at + x : M * N;
    }
    static constexpr std::ptrdiff_t back(std::ptrdiff_t at, std::ptrdiff_t x)
    {
        if (x >= 0) {
            return (x <= at) ? at - x : M * N;
        }
        return (x >= at - M * N) ? at - x : M * N;
    }

    T d[M + 1][N];
};

#endif // define      INC_COBMATRIX
