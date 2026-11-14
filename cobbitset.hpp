/* Compile time bound bitset. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBBITSET)
#define      INC_COBBITSET


#include "cobi.hpp"

#include <bit>
#include <cstdint>


// Bits indexed by a range bound integer
template <int N>
struct cobbitset {
    static_assert(N > 0, "Bitset must have some bits");

    using index = cobint<0, N - 1>;

    constexpr bool test(index i) const { return ((w[i.get() / 64] >> (i.get() % 64)) & 1u) != 0; }
    constexpr void set(index i, bool v = true)
    {
        const std::uint64_t bit = std::uint64_t{ 1 } << (i.get() % 64);
        if (v) {
            w[i.get() / 64] |= bit;
        }
        else {
            w[i.get() / 64] &= ~bit;
        }
    }
    constexpr void reset(index i) { set(i, false); }
    constexpr void flip(index i) { w[i.get() / 64] ^= std::uint64_t{ 1 } << (i.get() % 64); }

    constexpr void set()
    {
        for (auto& x : w) { x = ~std::uint64_t{ 0 }; }
        mask();
    }
    constexpr void reset()
    {
        for (auto& x : w) { x = 0; }
    }
    constexpr void flip()
    {
        for (auto& x : w) { x = ~x; }
        mask();
    }
    constexpr bool maybe_set(int i, bool v = true)
    {
        index idx;
        if (idx.be(i)) {
            set(idx, v);
            return true;
        }
        return false;
    }

    constexpr unsigned count() const
    {
        unsigned c = 0;
        for (auto x : w) { c += static_cast<unsigned>(std::popcount(x)); }
        return c;
    }
    constexpr bool any() const { return count() > 0; }
    constexpr bool none() const { return count() == 0; }
    constexpr bool all() const { return count() == static_cast<unsigned>(N); }
    static constexpr unsigned size() { return N; }

    constexpr cobbitset& operator&=(cobbitset const& x)
    {
        for (int i = 0; i < words; ++i) { w[i] &= x.w[i]; }
        return *this;
    }
    constexpr cobbitset& operator|=(cobbitset const& x)
    {
        for (int i = 0; i < words; ++i) { w[i] |= x.w[i]; }
        return *this;
    }
    constexpr cobbitset& operator^=(cobbitset const& x)
    {
        for (int i = 0; i < words; ++i) { w[i] ^= x.w[i]; }
        return *this;
    }
    constexpr cobbitset operator~() const
    {
        cobbitset r = *this;
        r.flip();
        return r;
    }
    friend constexpr cobbitset operator&(cobbitset x, cobbitset const& y) { return x &= y; }
    friend constexpr cobbitset operator|(cobbitset x, cobbitset const& y) { return x |= y; }
    friend constexpr cobbitset operator^(cobbitset x, cobbitset const& y) { return x ^= y; }
    constexpr bool operator==(cobbitset const& x) const
    {
        for (int i = 0; i < words; ++i) {
            if (w[i] != x.w[i]) {
                return false;
            }
        }
        return true;
    }

    // The lowest set bit
    template <template <class> class V> constexpr V<index> first() const { return from<V>(0); }
    // The lowest set bit above `after`
    template <template <class> class V> constexpr V<index> next(index after) const
    {
        return from<V>(after.get() + 1);
    }
    // Calls f(index) for each set bit, ascending
    template <class F> constexpr void for_each(F&& f) const
    {
        for (int b = 0; b < N; ++b) {
            index i;
            if (i.be(b) && test(i)) {
                f(i);
            }
        }
    }

private:
    static constexpr int words = (N + 63) / 64;

    // The unused high bits of the last word are always 0
    constexpr void mask()
    {
        if constexpr ((N % 64) != 0) {
            w[words - 1] &= (std::uint64_t{ 1 } << (N % 64)) - 1;
        }
    }

    // The lowest set bit at `start` or above
    template <template <class> class V> constexpr V<index> from(int start) const
    {
        if (start >= N) {
            return {};
        }
        for (int i = start / 64; i < words; ++i) {
            std::uint64_t x = w[i];
            if (i == start / 64) {
                x &= ~std::uint64_t{ 0 } << (start % 64);
            }
            if (x != 0) {
                index r;
                r.be(i * 64 + std::countr_zero(x));
                return r;
            }
        }
        return {};
    }

    std::uint64_t w[(N + 63) / 64] = {};
};


#endif // !defined(INC_COBBITSET)
