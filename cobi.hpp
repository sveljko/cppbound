/* Compile time bound integers. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBI)
#define INC_COBI

#include <type_traits>
#include <limits>
#include <algorithm>
#include <compare>
#include <cstddef>
#include <iosfwd>
#include <utility>
#include <iterator>


namespace cobhlp {

// Greatest possible magnitude of `x % y` for `y` in [D2, G2] (which does not
// contain zero): one less than the greatest magnitude of the divisor.
template <class T> constexpr T modmag(T D2, T G2)
{
    return (D2 > 0) ? G2 - 1 : -(D2 + 1);
}
template <class T> constexpr bool modexact(T D1, T G1, T D2, T G2)
{
    if ((D1 != G1) || (D2 != G2) || (D2 == 0)) {
        return false;
    }
    if (std::is_signed<T>::value) {
        return (D1 != std::numeric_limits<T>::lowest()) || (D2 != static_cast<T>(-1));
    }
    return true;
}
// Bounds of `x % y`, for `x` in [D1, G1] and `y` in [D2, G2]. The result has
// the sign of the dividend and a magnitude smaller than the divisor's and
// not greater than the dividend's.
template <class T> constexpr T modlo(T D1, T G1, T D2, T G2)
{
    if (modexact(D1, G1, D2, G2)) {
        return D1 % D2;
    }
    return (D1 >= 0) ? T(0) : std::max<T>(D1, -modmag(D2, G2));
}
template <class T> constexpr T modhi(T D1, T G1, T D2, T G2)
{
    if (modexact(D1, G1, D2, G2)) {
        return D1 % D2;
    }
    return (G1 <= 0) ? T(0) : std::min<T>(G1, modmag(D2, G2));
}
// Bounds of the absolute value of x in [D, G]
template <class T> constexpr T abslo(T D, T G)
{
    return (D >= 0) ? D : (G <= 0) ? static_cast<T>(-G) : T(0);
}
template <class T> constexpr T abshi(T D, T G)
{
    return (D >= 0) ? G : (G <= 0) ? static_cast<T>(-D) : std::max<T>(static_cast<T>(-D), G);
}

} // namespace cobhlp


template<class T, T D, T G = D> struct cobrng {
    static_assert(std::is_integral<T>::value, "T must be an integer type");
    static_assert(D <= G, "Upper bound must be at least as high as lower");
    static_assert(std::numeric_limits<T>::max() > G, "Upper bound must be less than maximum value");
    T i;

public:
    using value_type = T;
    using reference = T;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr cobrng() : i(D) {}

    static constexpr cobrng begin() { return cobrng{}; }
    static constexpr cobrng end() { cobrng x; x.i = G+1; return x; }

    constexpr cobrng& operator++() { i = (i <= G) ? i+1 : i; return *this; }
    constexpr cobrng operator++(int) { cobrng t = *this; ++*this; return t; }
    constexpr cobrng& operator--() { i = (i > D) ? i-1 : i; return *this; }

    constexpr const T operator*() const { return i; }
    constexpr bool operator==(cobrng<T,D,G> x) const { return i == x.i; }
};


template <class T, T D, T G = D> struct cobi {
    static_assert(std::is_integral<T>::value, "T must be an integer type");
    static_assert(D <= G, "Upper bound must be at least as high as lower");
    static constexpr T DD = D;
    static constexpr T GG = G;

    constexpr cobi()
        : i(D)
    {
    }

    static constexpr cobrng<T,D,G> range() {
        return cobrng<T,D,G>{};
    }

    template <T PD, T PG>
        requires((D <= PD) && (G >= PG))
    constexpr cobi(cobi<T, PD, PG> const& a)
        : i(a.get())
    {
    }
    template <T PD, T PG>
        requires((D <= PD) && (G >= PG))
    constexpr cobi& operator=(cobi<T, PD, PG> a)
    {
        i = a.get();
        return *this;
    }
    constexpr bool be(T x)
    {
        if ((x >= D) && (x <= G)) {
            i = x;
            return true;
        }
        return false;
    }
    static constexpr bool could(T x) { return ((x >= D) && (x <= G)); }
    template <template <class> class V>
    static constexpr V<cobi<T, D, G>> be(T x)
    {
        cobi<T, D, G> rzlt;
        if (rzlt.be(x)) {
            return rzlt;
        }
        else {
            return {};
        }
    }
    constexpr bool advance()
    {
        if (i < G) {
            ++i;
            return true;
        }
        return false;
    }
    constexpr bool ebb()
    {
        if (i > D) {
            --i;
            return true;
        }
        return false;
    }

    constexpr static cobi<T, D, G> greatest()
    {
        cobi<T, D, G> rslt;
        rslt.be(G);
        return rslt;
    }
    constexpr static cobi<T, D, G> smallest()
    {
        cobi<T, D, G> rslt;
        rslt.be(D);
        return rslt;
    }

    template <T D1, T G1> constexpr auto operator<=>(cobi<T, D1, G1> x) const { return i <=> x.get(); }
    constexpr auto operator<=>(T x) const { return i <=> x; }

    // Conversion to another integer type, if the whole range fits
    template <class U>
        requires(std::in_range<U>(D) && std::in_range<U>(G))
    constexpr cobi<U, static_cast<U>(D), static_cast<U>(G)> as() const
    {
        cobi<U, static_cast<U>(D), static_cast<U>(G)> r;
        r.be(static_cast<U>(i));
        return r;
    }

    // The value of the range that is nearest to `x`
    static constexpr cobi clamp(T x) { cobi r; r.i = (x < D) ? D : (x > G) ? G : x; return r; }
    // Like advance()/ebb(), but going around
    constexpr void advance_wrap() { i = (i < G) ? i + 1 : D; }
    constexpr void ebb_wrap() { i = (i > D) ? i - 1 : G; }

    constexpr bool operator==(T x) const { return i == x; }
    constexpr bool operator!=(T x) const { return !(i == x); }
    constexpr bool operator<(T x) const { return i < x; }
    constexpr bool operator<=(T x) const { return i <= x; }
    constexpr bool operator>(T x) const { return !(i <= x); }
    constexpr bool operator>=(T x) const { return !(i < x); }

    template <class U, U D1, U G1, U D2, U G2>
    constexpr friend bool operator==(cobi<U, D1, G1> x, cobi<U, D2, G2> y);
    template <class U, U D1, U G1>
    constexpr bool operator!=(cobi<U, D1, G1> x) const
    {
        return !(*this == x);
    }
    template <class U, U D1, U G1, U D2, U G2>
    constexpr friend bool operator<(cobi<U, D1, G1> x, cobi<U, D2, G2> y);
    template <class U, U D1, U G1>
    constexpr bool operator<=(cobi<U, D1, G1> x) const
    {
        return (*this < x) || (*this == x);
    }
    template <class U, U D1, U G1>
    constexpr bool operator>(cobi<U, D1, G1> x) const
    {
        return !(*this <= x);
    }
    template <class U, U D1, U G1>
    constexpr bool operator>=(cobi<U, D1, G1> x) const
    {
        return !(*this < x);
    }

    template <class U, U D1, U G1>
    friend constexpr  cobi<U, -G1, -D1> operator-(cobi<U, D1, G1> x);

    template <class U, U D1, U G1, U D2, U G2>
    friend constexpr cobi<U, D1 + D2, G1 + G2> operator+(cobi<U, D1, G1> x, cobi<U, D2, G2> y);

    template <class U, U D1, U G1, U D2, U G2>
    friend constexpr cobi<U, D1 - G2, G1 - D2> operator-(cobi<U, D1, G1> x, cobi<U, D2, G2> y);

    template <class U, U D1, U G1, U D2, U G2>
    friend constexpr cobi<U,
                std::min(std::min(D1* D2, G1* G2), std::min(D1* G2, G1* D2)),
                std::max(std::max(D1* D2, G1* G2), std::max(D1* G2, G1* D2))>
    operator*(cobi<U, D1, G1> x, cobi<U, D2, G2> y);

    template <class U, U D1, U G1, U D2, U G2>
    friend constexpr cobi<U,
                std::min(std::min(D1 / D2, G1 / G2), std::min(D1 / G2, G1 / D2)),
                std::max(std::max(D1 / D2, G1 / G2), std::max(D1 / G2, G1 / D2))>
    operator/(cobi<U, D1, G1> x, cobi<U, D2, G2> y);

    template <class U, U D1, U G1, U D2, U G2>
    friend constexpr cobi<U,
                cobhlp::modlo<U>(D1, G1, D2, G2),
                cobhlp::modhi<U>(D1, G1, D2, G2)>
    operator%(cobi<U, D1, G1> x, cobi<U, D2, G2> y);

    constexpr T get() const { return i; }

private:
    T i;
};

template <int D, int G = D> using cobint = cobi<int, D, G>;

template <int D, int G = D> constexpr auto cobic = cobi<int, D, G>{};


template <class T, T D1, T G1, T D2, T G2>
constexpr bool operator==(cobi<T, D1, G1> x, cobi<T, D2, G2> y)
{
    if constexpr ((G1 < D2) || (D1 > G2)) {
        return false;
    }
    return x.i == y.i;
}

template <class T, T D, T G> constexpr bool operator==(T x, cobi<T, D, G> y)
{
    return y == x;
}
template <class T, T D, T G> constexpr bool operator!=(T x, cobi<T, D, G> y)
{
    return y != x;
}
template <class T, T D, T G> constexpr bool operator<(T x, cobi<T, D, G> y)
{
    return y > x;
}
template <class T, T D, T G> constexpr bool operator<=(T x, cobi<T, D, G> y)
{
    return y >= x;
}
template <class T, T D, T G> constexpr bool operator>(T x, cobi<T, D, G> y)
{
    return y < x;
}
template <class T, T D, T G> constexpr bool operator>=(T x, cobi<T, D, G> y)
{
    return y <= x;
}


template <class T, T D1, T G1, T D2, T G2>
inline constexpr bool operator<(cobi<T, D1, G1> x, cobi<T, D2, G2> y)
{
    if constexpr (G1 < D2) {
        return true;
    }
    else if constexpr (D1 > G2) {
        return false;
    }
    return x.i < y.i;
}



template <class T, T D, T G> constexpr cobi<T, -G, -D> operator-(cobi<T, D,G> x) {
    static_assert(std::is_signed<T>::value, "Can't negate an unsigned integer");
    cobi<T, -G, -D> r;
    r.i = -x.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, D1+D2, G1+G2> operator+(cobi<T, D1,G1> x, cobi<T, D2,G2> y) {
#if defined(_MSC_VER)
    constexpr auto upperG = std::max(G1, G2);
    constexpr auto lowerG = std::min(G1, G2);
    if constexpr (upperG >= 0) {
        static_assert(std::numeric_limits<T>::max() - upperG >= lowerG,
                      "Addition Overflow");
    }
    constexpr auto upperD = std::max(D1, D2);
    constexpr auto lowerD = std::min(D1, D2);
    if constexpr (lowerD < 0) {
        static_assert(std::numeric_limits<T>::lowest() - lowerD <= upperD,
                      "Addition Underflow");
    }
#endif // defined(_MSC_VER)
    // Unsigned integers wrap, so this is not caught as undefined behavior
    static_assert(std::is_signed<T>::value || (std::numeric_limits<T>::max() - G1 >= G2),
                  "Addition Overflow");
    cobi<T, D1 + D2, G1 + G2> r;
    r.i = x.i + y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, D1-G2, G1-D2> operator-(cobi<T, D1,G1> x, cobi<T, D2,G2> y) {
    // Unsigned integers wrap, so this is not caught as undefined behavior
    static_assert(std::is_signed<T>::value || (D1 >= G2), "Subtraction Underflow");
    cobi<T, D1 - G2, G1 - D2> r;
    r.i = x.i - y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, std::min(std::min(D1*D2, G1*G2), std::min(D1*G2, G1*D2)),
        std::max(std::max(D1*D2, G1*G2), std::max(D1*G2, G1*D2))>
operator*(cobi<T, D1,G1> x, cobi<T, D2,G2> y) {
#if defined(_MSC_VER)
    if constexpr ((G1 > 0) && (G2 > 0)) {
        static_assert(std::numeric_limits<T>::max() / G1 >= G2,
                      "Multiplication Overflow");
    }
    else if constexpr ((G1 > 0) && (D2 < 0)) {
        static_assert(std::numeric_limits<T>::lowest() / D2 >= G1,
                      "Multiplication Underflow");
    }
    else if constexpr ((G1 < 0) && (G2 > 0)) {
        static_assert(std::numeric_limits<T>::lowest() / G1 >= G2,
                      "Multiplication Underflow");
    }
    if constexpr ((D1 < 0) && (D2 < 0)) {
        static_assert(std::numeric_limits<T>::max() / D1 <= D2,
                      "Multiplication Overflow");
    }
#endif // defined(_MSC_VER)
    // Unsigned integers wrap, so this is not caught as undefined behavior
    static_assert(std::is_signed<T>::value || (G1 == 0)
                      || (std::numeric_limits<T>::max() / G1 >= G2),
                  "Multiplication Overflow");
    cobi<T,
         std::min(std::min(D1 * D2, G1 * G2), std::min(D1 * G2, G1 * D2)),
         std::max(std::max(D1 * D2, G1 * G2), std::max(D1 * G2, G1 * D2))>
        r;
    r.i = x.i * y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, std::min(std::min(D1/D2, G1/G2), std::min(D1/G2, G1/D2)),
        std::max(std::max(D1/D2, G1/G2), std::max(D1/G2, G1/D2))>
operator/(cobi<T, D1,G1> x, cobi<T, D2,G2> y)
{
    static_assert(!std::is_signed<T>::value || (D1 > std::numeric_limits<T>::lowest())
                      || (G2 < -1) || (D2 > -1),
                  "Division Overflow");
    static_assert((G2 < 0) || (D2 > 0), "Possible division by zero");

    cobi<T,
         std::min(std::min(D1 / D2, G1 / G2), std::min(D1 / G2, G1 / D2)),
         std::max(std::max(D1 / D2, G1 / G2), std::max(D1 / G2, G1 / D2))>
        r;
    r.i = x.i / y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, cobhlp::modlo<T>(D1, G1, D2, G2), cobhlp::modhi<T>(D1, G1, D2, G2)>
operator%(cobi<T, D1,G1> x, cobi<T, D2,G2> y)
{
    static_assert(!std::is_signed<T>::value || (D1 > std::numeric_limits<T>::lowest())
                      || (G2 < -1) || (D2 > -1),
                  "Modulo/Division Overflow");
    static_assert((G2 < 0) || (D2 > 0), "Possible modulo/division by zero");

    cobi<T, cobhlp::modlo<T>(D1, G1, D2, G2), cobhlp::modhi<T>(D1, G1, D2, G2)> r;
    r.i = x.i % y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, std::max(D1, D2), std::max(G1, G2)> max(cobi<T, D1, G1> x, cobi<T, D2, G2> y)
{
    cobi<T, std::max(D1, D2), std::max(G1, G2)> r;
    r.be(std::max(x.get(), y.get()));
    return r;
}

template <class T, T D1, T G1, T D2, T G2>
constexpr cobi<T, std::min(D1, D2), std::min(G1, G2)> min(cobi<T, D1, G1> x, cobi<T, D2, G2> y)
{
    cobi<T, std::min(D1, D2), std::min(G1, G2)> r;
    r.be(std::min(x.get(), y.get()));
    return r;
}

template <class T, T D, T G>
constexpr cobi<T, cobhlp::abslo<T>(D, G), cobhlp::abshi<T>(D, G)> abs(cobi<T, D, G> x)
{
    static_assert(!std::is_signed<T>::value || (D > std::numeric_limits<T>::lowest()),
                  "Absolute value overflow");
    cobi<T, cobhlp::abslo<T>(D, G), cobhlp::abshi<T>(D, G)> r;
    r.be((x.get() < 0) ? static_cast<T>(-x.get()) : x.get());
    return r;
}

template <class CH, class TR, class T, T D, T G>
std::basic_ostream<CH, TR>& operator<<(std::basic_ostream<CH, TR>& os, cobi<T, D, G> x)
{
    return os << +x.get(); // the plus: character types are printed as numbers
}


template<class T, T D, T G = D> struct cobirange {
    static_assert(std::is_integral<T>::value, "T must be an integer type");
    static_assert(D <= G, "Upper bound must be at least as high as lower");
    static_assert(std::numeric_limits<T>::max() > G, "Upper bound must be less than maximum value");
    cobi<T,D,G+1> i;

public:
    using value_type = cobi<T,D,G>;
    using reference = cobi<T,D,G>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr cobirange() {}
    constexpr cobirange(cobi<T,D,G+1> x) { i = x; }

    static constexpr cobirange begin() { return cobi<T,D,G+1>::smallest(); }
    static constexpr cobirange end() { return cobi<T,D,G+1>::greatest(); }

    constexpr cobirange& operator++() { i.advance(); return *this; }
    constexpr cobirange operator++(int) { cobirange t = *this; ++*this; return t; }
    constexpr cobirange& operator--() { i.ebb(); return *this; }

    constexpr const cobi<T,D,G> operator*() const { cobi<T,D,G> r; r.be(i.get()); return r; }
    constexpr bool operator==(cobirange x) const { return i == x.i; }
};


#endif // !defined(INC_COBI)
