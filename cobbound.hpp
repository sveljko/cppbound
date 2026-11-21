/* What the compile time bound containers are built on. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBBOUND)
#define      INC_COBBOUND


#include "bndarray.hpp"
#include "cobhlp.hpp"
#include "cobi.hpp"
#include "cobstore.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>


// The containers are written once, for a "bound": a type that says how many
// slots there are and gives the types to index and keep them with. This is
// the bound known at compile time; `atbbound` is the one set at run time.
template <int N> struct cobbound {
    static_assert(N > 0, "Must have some capacity");

    using size_type = unsigned;
    using index     = cobint<0, N - 1>;   // what the user indexes with: always a slot
    using slot      = index;              // what the containers index with
    using link      = cobint<0, N>;       // a slot, or `nil`
    using level     = cobint<0, static_cast<int>(cobhlp::log2(N))>;   // of a skiplist

    static constexpr auto nil = cobic<N>;

    static constexpr bool fixed = true;   // the size is known at compile time

    static constexpr unsigned size() noexcept { return N; }
    static constexpr cobirange<int, 0, N - 1> slots() { return {}; }
    static constexpr cobirange<int, 0, N - 1> indices() { return {}; }

    template <class U> using array    = bndarray<U, cobbound>;                       // a U for each slot
    template <class U> using values   = bndarray<U, cobbound>;                       // an array with as many elements
    template <class U> using store    = cobstore<U, N>;                              // room for a U in each slot
    template <class U> using perlevel = bndarray<U, cobbound<level::GG + 1>>;        // a U for each level

    // A U for each level of each slot
    template <class U> struct table {
        constexpr U    get(slot i, level l) const { return d[i.get()][l.get()]; }
        constexpr void set(slot i, level l, U const& u) { d[i.get()][l.get()] = u; }

    private:
        U d[N][level::GG + 1];
    };

    // One U more than there are slots, value initialised
    template <class U> struct buffer {
        constexpr buffer() = default;
        constexpr explicit buffer(cobhlp::nowhere_t) {}
        // A value for each slot
        template <class... V>
            requires((sizeof...(V) == N) && (std::is_convertible_v<V, U> && ...))
        constexpr buffer(V&&... v) : d{ U(std::forward<V>(v))..., U{} } {}

        static constexpr std::size_t count() noexcept { return N; }     // not counting the one more

        constexpr U*       data() noexcept { return d; }
        constexpr U const* data() const noexcept { return d; }

    private:
        U d[N + 1] = {};
    };

    // A U for each pair of a slot of this bound and a slot of `C`, row by
    // row, and one more: the safety spot of the iterators
    template <class U, class C> struct flat {
        constexpr U*       data() noexcept { return d; }
        constexpr U const* data() const noexcept { return d; }

    private:
        U d[N * C::size() + 1];
    };

    // A bit for each slot, all clear
    struct words {
        static constexpr unsigned count() noexcept { return (N + 63) / 64; }
        constexpr std::uint64_t&       operator[](unsigned i) { return w[i]; }
        constexpr std::uint64_t const& operator[](unsigned i) const { return w[i]; }

    private:
        std::uint64_t w[(N + 63) / 64] = {};
    };
};


#endif // !defined(INC_COBBOUND)
