/* Compile time bound bitset. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBBITSET)
#define      INC_COBBITSET


#include "cobbound.hpp"

#include <bit>
#include <cstdint>
#include <utility>


// Bits indexed by an index that is known to be within the bound
template <class B>
struct bndbitset {
    using size_type = typename B::size_type;
    using index     = typename B::index;

    constexpr bool test(index i) const { return bit(at(i)); }
    constexpr void set(index i, bool v = true) { put(at(i), v); }
    constexpr void reset(index i) { put(at(i), false); }
    constexpr void flip(index i) { put(at(i), !bit(at(i))); }

    constexpr void set()
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] = ~std::uint64_t{ 0 }; }
        mask();
    }
    constexpr void reset()
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] = 0; }
    }
    constexpr void flip()
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] = ~w[i]; }
        mask();
    }
    // false if there is no such bit
    template <class U> constexpr bool maybe_set(U i, bool v = true)
    {
        if (std::cmp_less(i, 0) || !std::cmp_less(i, B::size())) {
            return false;
        }
        put(static_cast<size_type>(i), v);
        return true;
    }

    constexpr size_type count() const
    {
        size_type c = 0;
        for (unsigned i = 0; i < w.count(); ++i) { c += static_cast<size_type>(std::popcount(w[i])); }
        return c;
    }
    constexpr bool any() const { return count() > 0; }
    constexpr bool none() const { return count() == 0; }
    constexpr bool all() const { return count() == B::size(); }
    static constexpr size_type size() { return B::size(); }

    constexpr bndbitset& operator&=(bndbitset const& x)
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] &= x.w[i]; }
        return *this;
    }
    constexpr bndbitset& operator|=(bndbitset const& x)
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] |= x.w[i]; }
        return *this;
    }
    constexpr bndbitset& operator^=(bndbitset const& x)
    {
        for (unsigned i = 0; i < w.count(); ++i) { w[i] ^= x.w[i]; }
        return *this;
    }
    constexpr bndbitset operator~() const
    {
        bndbitset r = *this;
        r.flip();
        return r;
    }
    friend constexpr bndbitset operator&(bndbitset x, bndbitset const& y) { return x &= y; }
    friend constexpr bndbitset operator|(bndbitset x, bndbitset const& y) { return x |= y; }
    friend constexpr bndbitset operator^(bndbitset x, bndbitset const& y) { return x ^= y; }
    constexpr bool operator==(bndbitset const& x) const
    {
        for (unsigned i = 0; i < w.count(); ++i) {
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
        return from<V>(static_cast<size_type>(after.get()) + 1);
    }
    // Calls f(index) for each set bit, ascending
    template <class F> constexpr void for_each(F&& f) const
    {
        for (auto i : B::indices()) {
            if (test(i)) {
                f(i);
            }
        }
    }

private:
    // The number of the bit, which is less than the size
    static constexpr size_type at(index i) { return static_cast<size_type>(i.get()); }
    constexpr bool bit(size_type i) const { return ((w[static_cast<unsigned>(i / 64)] >> (i % 64)) & 1u) != 0; }
    constexpr void put(size_type i, bool v)
    {
        const std::uint64_t mask = std::uint64_t{ 1 } << (i % 64);
        if (v) {
            w[static_cast<unsigned>(i / 64)] |= mask;
        }
        else {
            w[static_cast<unsigned>(i / 64)] &= ~mask;
        }
    }

    // The unused high bits of the last word are always 0
    constexpr void mask()
    {
        if ((B::size() % 64) != 0) {
            w[w.count() - 1] &= (std::uint64_t{ 1 } << (B::size() % 64)) - 1;
        }
    }

    // The lowest set bit at `start` or above
    template <template <class> class V> constexpr V<index> from(size_type start) const
    {
        if (start >= B::size()) {
            return {};
        }
        for (unsigned i = static_cast<unsigned>(start / 64); i < w.count(); ++i) {
            std::uint64_t x = w[i];
            if (i == start / 64) {
                x &= ~std::uint64_t{ 0 } << (start % 64);
            }
            if (x != 0) {
                using number = decltype(std::declval<index>().get());
                return index::template be<V>(static_cast<number>(size_type{ i } * 64 + static_cast<unsigned>(std::countr_zero(x))));
            }
        }
        return {};
    }

    typename B::words w;
};

template <int N> using cobbitset = bndbitset<cobbound<N>>;


#endif // !defined(INC_COBBITSET)
