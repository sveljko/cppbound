/* Compile time bound integers. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBI)
#define INC_COBI

#include <type_traits>
#include <limits>
#include <algorithm>


template<class T, T D, T G = D> struct cobrng {
    static_assert(std::is_integral<T>::value, "T must be an integer type");
    static_assert(D <= G, "Upper bound must be at least as high as lower");
    static_assert(std::numeric_limits<T>::max() > G, "Upper bound must be less than maximum value");
    T i;

public:
    constexpr cobrng() : i(D) {}

    static constexpr cobrng begin() { return cobrng{}; }
    static constexpr cobrng end() { cobrng x; x.i = G+1; return x; }

    constexpr cobrng& operator++() { i = (i <= G) ? i+1 : i; return *this; }
    constexpr cobrng& operator--() { i = (i > D) ? i-1 : i; return *this; }

    constexpr const T operator*() const { return i; }
    constexpr bool operator==(cobrng<T,D,G> x) { return i == x.i; }
    constexpr bool operator!=(cobrng<T,D,G> x) { return i != x.i; }
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
    constexpr cobi(cobi<T, PD, PG> const& a)
        : i(a.get())
    {
        static_assert(D <= PD, "Out of lower bounds");
        static_assert(G >= PG, "Out of upper bounds");
    }
    template <T PD, T PG>
    constexpr cobi(cobi<T, PD, PG>&& a)
        : i(a.get())
    {
        static_assert(D <= PD, "Out of lower bounds");
        static_assert(G >= PG, "Out of upper bounds");
    }
    template <T PD, T PG> void operator=(cobi<T, PD, PG> a)
    {
        static_assert(D <= PD, "Out of lower bounds");
        static_assert(G >= PG, "Out of upper bounds");
        i = a.get();
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
                std::min(std::min(D1 % D2, G1 % G2), std::min(D1 % G2, G1 % D2)),
                std::max(std::max(D1 % D2, G1 % G2), std::max(D1 % G2, G1 % D2))>
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
    return y < x;
}
template <class T, T D, T G> constexpr bool operator<=(T x, cobi<T, D, G> y)
{
    return y <= x;
}
template <class T, T D, T G> constexpr bool operator>(T x, cobi<T, D, G> y)
{
    return y > x;
}
template <class T, T D, T G> constexpr bool operator>=(T x, cobi<T, D, G> y)
{
    return y >= x;
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
    cobi<T, D1 + D2, G1 + G2> r;
    r.i = x.i + y.i;
    return r;
}

template <class T, T D1, T G1, T D2, T G2>

constexpr cobi<T, D1-G2, G1-D2> operator-(cobi<T, D1,G1> x, cobi<T, D2,G2> y) {
    return x + (-y);
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
    static_assert((D1 > std::numeric_limits<T>::lowest()) || (G2 < -1) || (D2 > -1),
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
constexpr cobi<T, std::min(std::min(D1%D2, G1%G2), std::min(D1%G2, G1%D2)),
        std::max(std::max(D1%D2, G1%G2), std::max(D1%G2, G1%D2))>
operator%(cobi<T, D1,G1> x, cobi<T, D2,G2> y)
{
    static_assert((D1 > std::numeric_limits<T>::lowest()) || (G2 < -1) || (D2 > -1),
                  "Modulo/Division Overflow");
    static_assert((G2 < 0) || (D2 > 0), "Possible modulo/division by zero");

    cobi<T,
         std::min(std::min(D1 % D2, G1 % G2), std::min(D1 % G2, G1 % D2)),
         std::max(std::max(D1 % D2, G1 % G2), std::max(D1 % G2, G1 % D2))>
        r;
    r.i = x.i / y.i;
    return r;
}


template<class T, T D, T G = D> struct cobirange {
    static_assert(std::is_integral<T>::value, "T must be an integer type");
    static_assert(D <= G, "Upper bound must be at least as high as lower");
    static_assert(std::numeric_limits<T>::max() > G, "Upper bound must be less than maximum value");
    cobi<T,D,G+1> i;

public:
    using value_type = cobi<T,D,G+1>;

    constexpr cobirange() {}
    constexpr cobirange(value_type x) { i = x; }

    static constexpr cobirange begin() { return value_type::smallest(); }
    static constexpr cobirange end() { return value_type::greatest(); }

    constexpr cobirange& operator++() { i.advance(); return *this; }
    constexpr cobirange& operator--() { i.ebb(); return *this; }

    constexpr const cobi<T,D,G> operator*() const { cobi<T,D,G> r; r.be(i.get()); return r; }
    constexpr bool operator==(cobirange x) { return i == x.i; }
    constexpr bool operator!=(cobirange x) { return i != x.i; }
};


#endif // !defined(INC_COBI)
