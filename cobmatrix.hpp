/* Compile time bound matrix. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBMATRIX)
#define INC_COBMATRIX


#include "cobbound.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


// `R` is the bound of the rows and `C` the bound of the columns
template <class T, class R, class C> struct bndmatrix {
    using value_type      = T;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using reference       = T&;

    using row    = typename R::index;
    using column = typename C::index;
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

    static constexpr auto rows() { return R::size(); }
    static constexpr auto columns() { return C::size(); }

    constexpr bool empty() const noexcept { return total() == 0; }
    constexpr auto size() const noexcept { return R::size() * C::size(); }
    constexpr auto max_size() const noexcept { return size(); }

    constexpr ctaken grab(index i) const { return cell(i); }
    constexpr taken grab(index i) { return cell(i); }
    constexpr T get(index i) const { return cell(i); }
    constexpr T set(index i, T const& t) { return cell(i) = t; }
    constexpr T set(index i, T&& t) { return cell(i) = std::move(t); }
    template <class U1, class U2> constexpr bool maybe_set(U1 m, U2 n, T const& t)
    {
        if (std::cmp_less(m, 0) || !std::cmp_less(m, R::size()) || std::cmp_less(n, 0)
            || !std::cmp_less(n, C::size())) {
            return false;
        }
        d.data()[static_cast<std::size_t>(m) * C::size() + static_cast<std::size_t>(n)] = t;
        return true;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> get(index i, V U::*m) const
    {
        return cell(i).*m;
    }
    template <class V, class U = T>
    constexpr std::enable_if_t<std::is_class_v<U>, V> set(index i, V U::*m, V const& v)
    {
        return cell(i).*m = v;
    }

    // Every element a copy of v
    static constexpr bndmatrix filled(T const& v)
    {
        bndmatrix r;
        r.fill(v);
        return r;
    }

    // Element at index i is f(i)
    template <class F> static constexpr bndmatrix generate(F f)
    {
        bndmatrix r;
        for (auto m : R::indices()) {
            for (auto n : C::indices()) {
                r.cell({ m, n }) = f(index{ m, n });
            }
        }
        return r;
    }

    constexpr void fill(T const& v)
    {
        for (std::ptrdiff_t i = 0; i < total(); ++i) {
            d.data()[i] = v;
        }
    }

    constexpr bool operator==(bndmatrix const& x) const
    {
        return std::equal(d.data(), d.data() + total(), x.d.data());
    }

    constexpr T front() const { return d.data()[0]; }
    constexpr T back() const { return d.data()[(total() > 0) ? total() - 1 : 0]; }

    constexpr void swap(bndmatrix& x)
    {
        for (std::ptrdiff_t i = 0; i < total(); ++i) {
            std::swap(d.data()[i], x.d.data()[i]);
        }
    }

    constexpr typename C::template values<T> get_row(row r) const
    {
        typename C::template values<T> rslt;
        for (auto n : C::indices()) {
            rslt.set(n, cell({ r, n }));
        }
        return rslt;
    }
    constexpr void set_row(row r, typename C::template values<T> const& v)
    {
        for (auto n : C::indices()) {
            cell({ r, n }) = v.get(n);
        }
    }

    // The iterators differ in constness only: `M` is the matrix, `U` the
    // element. They go row by row and keep the offset from the first
    // element, in [0, size]; at the size, there is the safety spot.
    template <class M, class U> class iter {
        M*             r;
        std::ptrdiff_t at;

    public:
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = U*;
        using reference         = U&;
        using iterator_category = std::random_access_iterator_tag;

        iter() : r(&nowhere()), at(total()) {}
        // A constant iterator from one that is not
        template <class M2, class U2>
            requires(std::is_const_v<M> && !std::is_const_v<M2>)
        iter(iter<M2, U2> const& x) : r(x.r), at(x.at) {}

        iter& operator++()
        {
            if (at < total()) {
                ++at;
            }
            return *this;
        }
        iter operator++(int) { iter t = *this; ++*this; return t; }
        iter& operator--()
        {
            if (at > 0) {
                --at;
            }
            return *this;
        }
        iter operator--(int) { iter t = *this; --*this; return t; }
        iter operator+(difference_type x) const { return { r, forth(at, x) }; }
        iter& operator+=(difference_type x) { return *this = (*this) + x; }
        iter operator-(difference_type x) const { return { r, back(at, x) }; }
        iter& operator-=(difference_type x) { return *this = (*this) - x; }
        difference_type operator-(iter const& x) const { return (r == x.r) ? at - x.at : 0; }
        reference operator[](difference_type n) const { return *(*this + n); }
        friend iter operator+(difference_type n, iter const& i) { return i + n; }

        U& operator*() const { return r->d.data()[at]; }
        U* operator->() const { return &**this; }

        bool operator==(iter const& x) const { return (r == x.r) && (at == x.at); }
        bool operator<(iter const& x) const { return (r == x.r) && (at < x.at); }
        bool operator<=(iter const& x) const { return (r == x.r) && (at <= x.at); }
        bool operator>(iter const& x) const { return (r == x.r) && (at > x.at); }
        bool operator>=(iter const& x) const { return (r == x.r) && (at >= x.at); }

        friend struct bndmatrix;
        template <class, class> friend class iter;

    private:
        iter(M* r_, difference_type at_) : r(r_), at(at_) {}
    };
    using I              = iter<bndmatrix, T>;
    using CI             = iter<bndmatrix const, T const>;
    using iterator       = I;
    using const_iterator = CI;

    I  begin() { return I{ this, 0 }; }
    I  end() { return I{ this, total() }; }
    CI begin() const { return CI{ this, 0 }; }
    CI end() const { return CI{ this, total() }; }
    CI cbegin() const { return CI{ this, 0 }; }
    CI cend() const { return CI{ this, total() }; }

private:
    static bndmatrix& nowhere() { static thread_local bndmatrix none{}; return none; }

    static constexpr std::ptrdiff_t total()
    {
        return static_cast<std::ptrdiff_t>(R::size()) * static_cast<std::ptrdiff_t>(C::size());
    }
    constexpr T& cell(index i)
    {
        return d.data()[static_cast<std::size_t>(i.irow.get()) * C::size() + static_cast<std::size_t>(i.icolumn.get())];
    }
    constexpr T const& cell(index i) const
    {
        return d.data()[static_cast<std::size_t>(i.irow.get()) * C::size() + static_cast<std::size_t>(i.icolumn.get())];
    }

    // Moving an iterator at offset `at` (in [0, size]) by `x`. All that would
    // end up out of bounds lands on the safety spot (offset size).
    static constexpr std::ptrdiff_t forth(std::ptrdiff_t at, std::ptrdiff_t x)
    {
        if (x >= 0) {
            return (x <= total() - at) ? at + x : total();
        }
        return (x >= -at) ? at + x : total();
    }
    static constexpr std::ptrdiff_t back(std::ptrdiff_t at, std::ptrdiff_t x)
    {
        if (x >= 0) {
            return (x <= at) ? at - x : total();
        }
        return (x >= at - total()) ? at - x : total();
    }

    typename R::template flat<T, C> d;
};

template <class T, int M, int N> using cobmatrix = bndmatrix<T, cobbound<M>, cobbound<N>>;

#endif // define      INC_COBMATRIX
