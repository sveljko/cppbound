/* An optional that uses a sentinel value, so that it takes no more room than
 * what it holds. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_CONTINGENT)
#define      INC_CONTINGENT


#include "cobi.hpp"
#include "perhaps.hpp"

#include <bit>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>


namespace cobhlp {
// For when what is stored is the T itself
template <class T> struct stored_as_is {
    using stored = T;
    static constexpr stored   pack(T x) { return x; }
    static constexpr T const& unpack(stored const& s) { return s; }
};
} // namespace cobhlp

// The usual "nothing" for a type. Specialise it for your own types.
template <class T> struct nothing;

// pointers: nullptr
template <class T> struct nothing<T*> : cobhlp::stored_as_is<T*> {
    static constexpr T*   none() noexcept { return nullptr; }
    static constexpr bool is_none(T* const& p) noexcept { return p == nullptr; }
};

// floating point: NaN. It is kept as the bits of the number and recognised by
// them, never as a floating point value, so that it works also with
// -ffast-math, which lets the compiler assume that there are no NaNs.
template <> struct nothing<double> {
    using stored = std::uint64_t;
    static constexpr stored none() noexcept { return 0x7ff8000000000000ULL; }
    static constexpr bool   is_none(stored const& s) noexcept
    {
        return (s & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL;
    }
    static constexpr stored pack(double x) { return std::bit_cast<stored>(x); }
    static constexpr double unpack(stored const& s) { return std::bit_cast<double>(s); }
};
template <> struct nothing<float> {
    using stored = std::uint32_t;
    static constexpr stored none() noexcept { return 0x7fc00000U; }
    static constexpr bool   is_none(stored const& s) noexcept
    {
        return (s & 0x7fffffffU) > 0x7f800000U;
    }
    static constexpr stored pack(float x) { return std::bit_cast<stored>(x); }
    static constexpr float  unpack(stored const& s) { return std::bit_cast<float>(s); }
};

// a chosen value: contingent<int, nothing_is<-1>>
template <auto X> struct nothing_is : cobhlp::stored_as_is<decltype(X)> {
    static constexpr decltype(X) none() noexcept { return X; }
    static constexpr bool        is_none(decltype(X) const& x) noexcept { return x == X; }
};

// range bound integers: the integer just outside the range
template <class T, T D, T G> struct nothing<cobi<T, D, G>> {
    static_assert((G < std::numeric_limits<T>::max()) || (D > std::numeric_limits<T>::lowest()),
                  "No integer left to mean 'nothing'");
    using stored = T;
    static constexpr T    none() noexcept { return (G < std::numeric_limits<T>::max()) ? G + 1 : D - 1; }
    static constexpr bool is_none(T const& x) noexcept { return x == none(); }
    static constexpr T    pack(cobi<T, D, G> x) { return x.get(); }
    static constexpr cobi<T, D, G> unpack(T const& s) { cobi<T, D, G> r; r.be(s); return r; }
};


template <class T, class S = nothing<T>> class contingent {
    typename S::stored v;

public:
    constexpr contingent() : v(S::none()) {}
    constexpr contingent(T x) : v(S::pack(std::move(x))) {}

    constexpr bool has_value() const { return !S::is_none(v); }
    constexpr void reset() { v = S::none(); }

    // f is called with the value (never with the sentinel)
    template <class F> constexpr auto and_then(F&& f) const
    {
        using R = std::remove_cvref_t<std::invoke_result_t<F, decltype(S::unpack(v))>>;
        if constexpr (std::is_void_v<R>) {
            if (has_value()) std::invoke(std::forward<F>(f), S::unpack(v));
        } else {
            if (has_value()) return R(std::invoke(std::forward<F>(f), S::unpack(v)));
            else             return R{};
        }
    }
    template <class F> constexpr auto or_else(F&& f) const { return has_value() ? *this : std::forward<F>(f)(); }
    template <class F> constexpr auto transform(F&& f) const
    {
        using R = std::remove_cvref_t<std::invoke_result_t<F, decltype(S::unpack(v))>>;
        if (has_value()) {
            return perhaps<R>(std::invoke(std::forward<F>(f), S::unpack(v)));
        }
        return perhaps<R>{};
    }
    template <class U = T> constexpr T value_or(U&& x) const
    {
        return has_value() ? static_cast<T>(S::unpack(v)) : static_cast<T>(std::forward<U>(x));
    }

    constexpr bool operator==(contingent const& x) const
    {
        if (!has_value()) {
            return !x.has_value();
        }
        return x.has_value() && (S::unpack(v) == S::unpack(x.v));
    }
};

// For where a template with one parameter is needed: l.pop_front<contingently>()
template <class T> using contingently = contingent<T>;


// A NULIFY for cobtmrleanmill or cobtmrleanring made from a "nothing"
// policy (only meant for policies that store the `T` itself)
template <class T, class S = nothing<T>> struct nulifier {
    constexpr bool operator()(T& id) const
    {
        if (S::is_none(S::pack(id))) {
            return false;
        }
        id = S::unpack(S::none());
        return true;
    }
};


#endif // !defined(INC_CONTINGENT)
