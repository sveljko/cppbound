/* What the allocation time bound containers are built on. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBBOUND)
#define      INC_ATBBOUND


#include "atbdim.hpp"
#include "bndarray.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <new>
#include <utility>


namespace cobhlp {

// `n` objects on the heap, value initialised. There is no way to change `n`
// and no moving, so the objects are always there.
template <class U> class heap {
    std::size_t          n;
    std::unique_ptr<U[]> d;

public:
    explicit heap(std::size_t n_) : n(n_), d(new U[n_]()) {}
    heap(heap const& x) : n(x.n), d(new U[x.n]()) { std::copy(x.d.get(), x.d.get() + n, d.get()); }
    heap& operator=(heap const& x)
    {
        if (this != &x) {
            std::copy(x.d.get(), x.d.get() + std::min(n, x.n), d.get());
        }
        return *this;
    }
    U*          data() noexcept { return d.get(); }
    U const*    data() const noexcept { return d.get(); }
    std::size_t size() const noexcept { return n; }
};

} // namespace cobhlp


// The bound of the `atb` containers: the number of slots is the dimension
// of `Tag`, set once, at run time (see `atbdim`). As soon as any container
// of the tag is made (or one of its iterators), the dimension is looked at,
// so it can not be set any more. All the storage is on the heap, allocated
// when a container is made, and never again.
//
// What keeps this sound: everything is allocated with (at least) one slot
// more than the dimension, and what indexes it is at most the dimension.
template <class Tag> struct atbbound {
    using size_type = std::size_t;
    using index     = atbidx<Tag>;    // what the user indexes with: always a slot

    static constexpr bool fixed = false;    // the size is not known at compile time

    static std::size_t size() noexcept { return atbdim<Tag>::get(); }

    // What the containers index with: a slot, or the first slot if there is none
    class slot {
        std::size_t i = 0;

    public:
        slot() = default;
        slot(index x) : i(x.get()) {}

        bool be(std::size_t x)
        {
            if (x < size()) {
                i = x;
                return true;
            }
            return false;
        }
        bool advance()
        {
            if (i + 1 < size()) {
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
        void        advance_wrap() { i = (i + 1 < size()) ? i + 1 : 0; }
        std::size_t get() const { return i; }
        friend bool operator==(slot x, slot y) { return x.i == y.i; }
    };

    struct nil_t {};
    static constexpr nil_t nil{};

    // A slot, or `nil`
    class link {
        std::size_t i = 0;

    public:
        link() = default;
        link(slot x) : i(x.get()) {}
        link(index x) : i(x.get()) {}
        link(nil_t) : i(size()) {}

        bool be(std::size_t x)
        {
            if (x <= size()) {
                i = x;
                return true;
            }
            return false;
        }
        std::size_t get() const { return i; }
        friend bool operator==(link x, link y) { return x.i == y.i; }
    };

    // The level of a skiplist
    class level {
        unsigned i = 0;

        static unsigned top() { return static_cast<unsigned>(std::bit_width(size() | 1u)) - 1u; }

    public:
        bool be(std::size_t x)
        {
            if (x <= top()) {
                i = static_cast<unsigned>(x);
                return true;
            }
            return false;
        }
        bool advance()
        {
            if (i < top()) {
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
        unsigned     get() const { return i; }
        static level greatest()
        {
            level r;
            r.i = top();
            return r;
        }
    };

    // for (auto i : slots())
    struct every {
        class iterator {
            std::size_t at;

        public:
            using value_type        = slot;
            using difference_type   = std::ptrdiff_t;
            using pointer           = void;
            using reference         = slot;
            using iterator_category = std::input_iterator_tag;

            explicit iterator(std::size_t at_ = 0) : at(at_) {}
            iterator& operator++()
            {
                ++at;
                return *this;
            }
            iterator operator++(int) { iterator t = *this; ++at; return t; }
            slot     operator*() const
            {
                slot s;
                s.be(at);
                return s;
            }
            bool operator==(iterator const& x) const { return at == x.at; }
        };
        iterator begin() const { return iterator{ 0 }; }
        iterator end() const { return iterator{ size() }; }
    };
    static every slots() { return {}; }
    static auto  indices() { return index::range(); }

    // A U for each slot
    template <class U> class array {
        cobhlp::heap<U> d{ size() + 1 };

    public:
        U    get(slot i) const { return d.data()[i.get()]; }
        void set(slot i, U const& u) { d.data()[i.get()] = u; }
        void fill(U const& u) { std::fill(d.data(), d.data() + size(), u); }
    };
    // An array with as many elements
    template <class U> using values = bndarray<U, atbbound>;

    // Room for a U in each slot. Like `cobstore`: it does not know which
    // slots hold an object.
    template <class U> class store {
        union room {
            char none;
            U    some;
            room() : none() {}
            ~room() {}
        };
        std::unique_ptr<room[]> s{ new room[size() + 1] };

    public:
        store()                        = default;
        store(store const&)            = delete;
        store& operator=(store const&) = delete;

        template <class... A> void make(slot i, A&&... a)
        {
            ::new (static_cast<void*>(&s[i.get()].some)) U(std::forward<A>(a)...);
        }
        void     drop(slot i) { s[i.get()].some.~U(); }
        U&       at(slot i) { return s[i.get()].some; }
        U const& at(slot i) const { return s[i.get()].some; }
    };

    // A U for each level; no skiplist has more than 64 of them
    template <class U> class perlevel {
        U d[64] = {};

    public:
        U    get(level l) const { return d[l.get()]; }
        void set(level l, U const& u) { d[l.get()] = u; }
        void fill(U const& u) { std::fill(d, d + 64, u); }
    };

    // A U for each level of each slot
    template <class U> class table {
        std::size_t     levels = level::greatest().get() + 1u;
        cobhlp::heap<U> d{ (size() + 1) * levels };

    public:
        U    get(slot i, level l) const { return d.data()[i.get() * levels + l.get()]; }
        void set(slot i, level l, U const& u) { d.data()[i.get() * levels + l.get()] = u; }
    };

    // One U more than there are slots, value initialised
    template <class U> class buffer {
        cobhlp::heap<U> d{ size() + 1 };

    public:
        buffer() = default;
        // Just the one more, and without looking at the dimension
        explicit buffer(cobhlp::nowhere_t) : d(1) {}

        std::size_t count() const noexcept { return d.size() - 1; }    // not counting the one more
        U*       data() noexcept { return d.data(); }
        U const* data() const noexcept { return d.data(); }
    };

    // A U for each pair of a slot of this bound and a slot of `C`, row by
    // row, and one more: the safety spot of the iterators
    template <class U, class C> class flat {
        cobhlp::heap<U> d{ size() * C::size() + 1 };

    public:
        U*       data() noexcept { return d.data(); }
        U const* data() const noexcept { return d.data(); }
    };

    // A bit for each slot, all clear
    class words {
        cobhlp::heap<std::uint64_t> w{ (size() + 63) / 64 + 1 };

    public:
        static unsigned      count() noexcept { return static_cast<unsigned>((size() + 63) / 64); }
        std::uint64_t&       operator[](unsigned i) { return w.data()[i]; }
        std::uint64_t const& operator[](unsigned i) const { return w.data()[i]; }
    };
};


#endif // !defined(INC_ATBBOUND)
