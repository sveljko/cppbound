/* Allocation time bound dimension and index. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBDIM)
#define      INC_ATBDIM


#include <atomic>
#include <compare>
#include <cstddef>
#include <iterator>


// The dimension that belongs to Tag. Set it once, before it is used.
// It changes at most once, from "not set" to what it is from then on, so
// whoever looks at it, from whatever thread, sees the same.
template <class Tag> class atbdim {
    static constexpr std::size_t           unset = ~std::size_t{ 0 };
    static inline std::atomic<std::size_t> n{ unset };

public:
    // false if it was already set, or already used
    static bool set(std::size_t x) noexcept
    {
        std::size_t was = unset;
        return (x != unset) && n.compare_exchange_strong(was, x);
    }
    // Once looked at, it can not change any more: if not set, it is 0
    static std::size_t get() noexcept
    {
        std::size_t is = n.load(std::memory_order_relaxed);
        if (is == unset) {
            // Not set: it is 0 from now on, unless someone has just set it,
            // in which case `is` becomes what it was set to
            if (n.compare_exchange_strong(is, 0)) {
                return 0;
            }
        }
        return is;
    }
};


// A tag whose dimension is worked out from that of `Tag`, so it can not be
// set: nothing if that one is less than MIN, otherwise it is divided by DEN,
// multiplied by NUM and ADD is added. That is how what consists of several
// containers (such as a timer wheel) gives each its share.
template <class Tag, std::size_t NUM = 1, std::size_t DEN = 1, std::size_t ADD = 0, std::size_t MIN = 0>
struct atbpart {};

template <class Tag, std::size_t NUM, std::size_t DEN, std::size_t ADD, std::size_t MIN>
class atbdim<atbpart<Tag, NUM, DEN, ADD, MIN>> {
public:
    static bool        set(std::size_t) noexcept { return false; }
    static std::size_t get() noexcept
    {
        const std::size_t n = atbdim<Tag>::get();
        return (n < MIN) ? 0 : n / DEN * NUM + ADD;
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
