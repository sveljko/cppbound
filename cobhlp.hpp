/* Compile time bound helpers. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBHLP)
#define INC_COBHLP

#include <cstdint>
#include <utility>

namespace cobhlp {

// TODO avoid defining this ourselves
static constexpr unsigned log2(unsigned v)
{
    unsigned r = (v > 0xFFFFU) << 4;
    v >>= r;
    unsigned shift = (v > 0xFFU) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xFU) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3U) << 1;
    v >>= shift;
    r |= shift | (v >> 1);
    return r;
}

static constexpr unsigned long long pow(unsigned long long base, unsigned exponent)
{
    unsigned long long r = 1;
    for (unsigned i = 0; i < exponent; ++i) {
        r *= base;
    }
    return r;
}

template <class T> struct less {
    constexpr bool operator()(T const& l, T const& r) const { return l < r; }
};

// A small deterministic random number generator
struct xorshift32 {
    std::uint32_t s = 2463534242u;
    constexpr std::uint32_t operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
};
// What a map (or a cache) keeps for a key. The key can not be changed, the value can.
template <class K, class V> struct keyval {
    K const key;
    V       value;

    keyval() : key(), value() {}
    template <class... A> explicit keyval(K const& k, A&&... a) : key(k), value(std::forward<A>(a)...) {}
};

} // namespace cobhlp


#endif // !defined(INC_COBHLP)
