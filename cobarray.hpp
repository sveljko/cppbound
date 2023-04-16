/* Compile time bound arrays. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBARRAY)
#define      INC_COBARRAY

#include "cobi.hpp"

#include <type_traits>
#include <limits>
#include <algorithm>


template <class T, int N>
struct cobarray {
    static_assert(N > 0, "Array must have at least one element");

    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    constexpr bool empty() const noexcept { return false; }
    constexpr unsigned size() const noexcept { return N; }
    constexpr unsigned max_size() const noexcept { return N; }

    constexpr T get(cobi<int, 0, N-1> i) const {
        return d[i.get()];
    }
    constexpr T set(cobi<int, 0, N-1> i, T const& t)  {
        return d[i.get()] = t;
    }

    void fill(T const& v) {
        for (T* p = d; d < p + N; ++p) {
            *p = v;
        }
    }

    class I {
        cobarray* r;
        T* p;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        I(I const& x) : r(x.r), p(x.p) {}

        I& operator=(I& x) {
            r = x.r;
            p = x.p;
            return *this;
        }
        I& operator=(I&& x) {
            r = x.r;
            p = x.p;
            return *this;
        }
        I& operator++() {
            if (p - r->d < N) {
                ++p;
            }
            return *this;
        }
        I& operator--() {
            if (p > r->d) {
                --p;
            }
            return *this;
        }
        I operator+(difference_type x) const {
            if (p + x > r->d) {
                if (p + x < r->d + N) {
                    return { r, p + x };
                }
            }
            return { r, r->d + N };
        }
        I operator+=(difference_type x) {
            return (*this) + x;
        }
        I operator-(difference_type x) const {
            if (p - x > r->d) {
                if (p - x < r->d + N) {
                    return { r, p - x };
                }
            }
            return { r, r->d + N };
        }
        I operator-=(difference_type x) {
            return (*this) - x;
        }
        difference_type operator-(I const& x) const {
            return p - x.p;
        }

        T& operator*() { return *p; }
        T const& operator*() const { return *p; }
        T* operator->() { return p; }
        T const* operator->() const { return p; }

        bool operator==(I const& x) const { return (r == x.r) && (p == x.p); }
        bool operator!=(I const& x) const { return (r != x.r) || (p != x.p); }
        bool operator< (I const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(I const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator> (I const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(I const& x) const { return (r == x.r) && (p >= x.p); }

        friend class cobarray;

    protected:
        I(cobarray const* r_, T const* p_) : r(const_cast<cobarray*>(r_)), p(const_cast<T*>(p_)) {}
    };
    class CI {
        cobarray const* r;
        T const* p;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        CI(CI const& x) : r(x.r), p(x.p) {}

        CI& operator=(CI& x) {
            r = x.r;
            p = x.p;
            return *this;
        }
        CI& operator=(CI&& x) {
            r = x.r;
            p = x.p;
            return *this;
        }
        CI& operator++() {
            if (p - r->d < N) {
                ++p;
            }
            return *this;
        }
        CI& operator--() {
            if (p > r->d) {
                --p;
            }
            return *this;
        }
        CI operator+(difference_type x) const {
            if (p + x > r->d) {
                if (p + x < r->d + N) {
                    return { r, p + x };
                }
            }
            return { r, r->d + N };
        }
        CI operator+=(difference_type x) {
            return (*this) + x;
        }
        CI operator-(difference_type x) const {
            if (p - x > r->d) {
                if (p - x < r->d + N) {
                    return { r, p - x };
                }
            }
            return { r, r->d + N };
        }
        CI operator-=(difference_type x) {
            return (*this) - x;
        }
        difference_type operator-(CI const& x) const {
            return p - x.p;
        }

        T const& operator*() const { return *p; }
        T const* operator->() const { return p; }

        bool operator==(CI const& x) const { return (r == x.r) && (p == x.p); }
        bool operator!=(CI const& x) const { return (r != x.r) || (p != x.p); }
        bool operator< (CI const& x) const { return (r == x.r) && (p < x.p); }
        bool operator<=(CI const& x) const { return (r == x.r) && (p <= x.p); }
        bool operator> (CI const& x) const { return (r == x.r) && (p > x.p); }
        bool operator>=(CI const& x) const { return (r == x.r) && (p >= x.p); }

        friend class cobarray;

    protected:
        CI(cobarray const* r_, T const* p_) : r(r_), p(p_) {}
    };

    constexpr I begin() { return I{this, &d[0]}; }
    constexpr I end() { return I{this, d + N}; }
    constexpr CI cbegin() const { return CI{this, &d[0]}; }
    constexpr CI cend() const { return CI{this, d + N}; }

private:
    T d[N+1];
};

#endif // define      INC_COBARRAY
