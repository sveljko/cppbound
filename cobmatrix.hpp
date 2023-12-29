/* Compile time bound matrix. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBMATRIX)
#define INC_COBMATRIX


#include "cobi.hpp"


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
        row    row;
        column column;
    };

    constexpr bool     empty() const noexcept { return false; }
    constexpr unsigned size() const noexcept { return M * N; }
    constexpr unsigned max_size() const noexcept { return M * N; }

    constexpr T get(index i) const { return d[i.row.get()][i.column.get()]; }
    constexpr T set(index i, T const& t)
    {
        return d[i.row.get()][i.column.get()] = t;
    }
    constexpr bool maybe_set(int m, int n, T const& t)
    {
        index idx;
        if (idx.row.be(m) && idx.column.be(n)) {
            d[m][n] = t;
            return true;
        }
        return false;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> get(index i, V U::*m) const
    {
        return d[i.row.get()][i.column.get()].*m;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> set(index i, V U::*m, V const& v)
    {
        return d[i.row.get()][i.column.get()].*m = v;
    }

    void fill(T const& v)
    {
        for (auto p = begin(); p != end(); ++p) {
            *p = v;
        }
    }

    constexpr bool operator!=(cobmatrix const& x)
    {
        auto [i, j] = std::mismatch(begin(), end(), x.begin());
        return i != end();
    }

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

        I(I const& x)
            : r(x.r)
            , row(x.row)
            , p(x.p)
        {
        }

        I& operator=(I& x)
        {
            r   = x.r;
            row = x.row;
            p   = x.p;
            return *this;
        }
        I& operator=(I&& x)
        {
            r   = x.r;
            row = x.row;
            p   = x.p;
            return *this;
        }
        I& operator++()
        {
            if (row - r->d < M) {
                if (++p - row == N) {
                    ++row;
                    p = *row;
                }
            }
            return *this;
        }
        I& operator--()
        {
            if (row > r->d) {
                if (p == *row) {
                    --row;
                    p = row[N - 1];
                }
            }
            return *this;
        }
        I operator+(difference_type x) const
        {
            // Should optimize!
            I rslt = *this;
            for (unsigned i = 0; i < x; ++i) {
                ++rslt;
            }
            return rslt;
        }
        I operator+=(difference_type x) { return *this = (*this) + x; }
        I operator-(difference_type x) const
        {
            // Should optimize!
            I rslt = *this;
            for (unsigned i = 0; i < x; ++i) {
                --rslt;
            }
            return rslt;
        }
        I operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(I const& x) const { return p - x.p; }

        T& operator*() const { return *p; }
        T* operator->() const { return p; }

        bool operator==(I const& x) const { return (r == x.r) && (p == x.p); }
        bool operator!=(I const& x) const { return (r != x.r) || (p != x.p); }
        bool operator<(I const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(I const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator>(I const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(I const& x) const { return (r == x.r) && (p >= x.p); }

        friend struct cobmatrix;

    protected:
        I(cobmatrix const* r_, T (*row_)[20], T const* p_)
            : r(const_cast<cobmatrix*>(r_))
            , row(const_cast<T(*[20])>(row_))
            , p(const_cast<T*>(p_))
        {
        }
    };
    class CI {
        cobmatrix const* r;
        T const (*row)[N];
        T const* p;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;
        using iterator_category = std::random_access_iterator_tag;

        CI(CI const& x)
            : r(x.r)
            , row(x.row)
            , p(x.p)
        {
        }

        CI& operator=(CI& x)
        {
            r   = x.r;
            row = x.row;
            p   = x.p;
            return *this;
        }
        CI& operator=(CI&& x)
        {
            r   = x.r;
            row = x.row;
            p   = x.p;
            return *this;
        }
        CI& operator++()
        {
            if (row - r->d < M) {
                if (++p - row == N) {
                    ++row;
                    p = *row;
                }
            }
            return *this;
        }
        CI& operator--()
        {
            if (row > r->d) {
                if (p == *row) {
                    --row;
                    p = row[N - 1];
                }
            }
            return *this;
        }
        CI operator+(difference_type x) const
        {
            // Should optimize!
            I rslt = *this;
            for (unsigned i = 0; i < x; ++i) {
                ++rslt;
            }
            return rslt;
        }
        CI operator+=(difference_type x) { return *this = (*this) + x; }
        CI operator-(difference_type x) const
        {
            // Should optimize!
            I rslt = *this;
            for (unsigned i = 0; i < x; ++i) {
                --rslt;
            }
            return rslt;
        }
        CI operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(CI const& x) const { return p - x.p; }

        T const& operator*() const { return *p; }
        T const* operator->() const { return p; }

        bool operator==(CI const& x) const { return (r == x.r) && (p == x.p); }
        bool operator!=(CI const& x) const { return (r != x.r) || (p != x.p); }
        bool operator<(CI const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(CI const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator>(CI const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(CI const& x) const { return (r == x.r) && (p >= x.p); }

        friend struct cobmatrix;

    protected:
        CI(cobmatrix const* r_, T const (*row_)[N], T const* p_)
            : r(r_)
            , row(row_)
            , p(p_)
        {
        }
    };

    constexpr I  begin() { return I{ this, &d[0], &d[0][0] }; }
    constexpr I  end() { return I{ this, &d[M], &d[M][0] }; }
    constexpr CI begin() const { return CI{ this, &d[0], &d[0][0] }; }
    constexpr CI end() const { return CI{ this, &d[M], &d[M][0] }; }
    constexpr CI cbegin() const { return CI{ this, &d[0], &d[0][0] }; }
    constexpr CI cend() const { return CI{ this, &d[M], &d[M][0] }; }


private:
    T d[M + 1][N];
};

#endif // define      INC_COBMATRIX
