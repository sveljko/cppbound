/* Allocation time bound dimension and index. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBDIM)
#define      INC_ATBDIM


#include <compare>
#include <cstddef>
#include <iterator>


// The dimension that belongs to Tag. Set it once, before it is used.
// Not thread safe: set the dimension before starting threads.
template <class Tag> class atbdim {
    static inline std::size_t n      = 0;
    static inline bool        frozen = false;

public:
    // false if it was already set, or already used
    static bool set(std::size_t x) noexcept
    {
        if (frozen) {
            return false;
        }
        n      = x;
        frozen = true;
        return true;
    }
    // Once looked at, it can not change any more
    static std::size_t get() noexcept
    {
        frozen = true;
        return n;
    }
};


// An index that is valid for every container of the tag, forever: it can only
// be made after the dimension is frozen, and it is below it. There is no
// default constructor and no public constructor from an integer.
template <class Tag> class atbidx {
    std::size_t i;
    constexpr explicit atbidx(std::size_t x) : i(x) {}

public:
    template <template <class> class V> static V<atbidx> be(std::size_t x)
    {
        if (x < atbdim<Tag>::get()) {
            return atbidx{ x };
        }
        return {};
    }
    template <template <class> class V> static V<atbidx> first() { return be<V>(0); }
    template <template <class> class V> static V<atbidx> last()
    {
        const std::size_t n = atbdim<Tag>::get();
        return (n > 0) ? be<V>(n - 1) : V<atbidx>{};
    }
    bool advance()
    {
        if (i + 1 < atbdim<Tag>::get()) {
            ++i;
            return true;
        }
        return false;
    }
    bool ebb()
    {
        if (i > 0) {
            --i;
            return true;
        }
        return false;
    }
    constexpr std::size_t get() const { return i; }
    constexpr auto operator<=>(atbidx const&) const = default;

    // for (auto i : atbidx<Tag>::range())
    class walk {
    public:
        class iterator {
            std::size_t pos = 0;
            friend class walk;
            explicit iterator(std::size_t p) : pos(p) {}

        public:
            using value_type        = atbidx;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = atbidx;
            using iterator_category = std::input_iterator_tag;

            iterator() = default;

            iterator& operator++()
            {
                ++pos;
                return *this;
            }
            iterator operator++(int)
            {
                iterator t = *this;
                ++pos;
                return t;
            }
            atbidx operator*() const { return atbidx{ pos }; }
            bool   operator==(iterator const& x) const { return pos == x.pos; }
        };

        iterator begin() const { return iterator{ 0 }; }
        iterator end() const { return iterator{ atbdim<Tag>::get() }; }
    };
    static walk range() { return walk{}; }
};


#endif // !defined(INC_ATBDIM)
