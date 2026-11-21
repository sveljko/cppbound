/* Compile time bound flat set. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFLATSET)
#define      INC_COBFLATSET


#include "cobvector.hpp"

#include <utility>    // std::pair


// A set of unique elements kept in order in a vector, one next to another.
// So there is nothing but the elements (and their number), and an element
// is found in O(log n) steps, in the worst case too, over memory that is all
// in one place. The price: to insert or erase one, those after it are moved,
// which is O(n), and nothing stays where it was - an iterator or an index
// is good only until the set is changed.
// For what is mostly looked up, or is small. Otherwise, see `cobtree`.
template <class T, class B, class CMP = cobhlp::less<T>>
struct bndflatset : private bndvector<T, B> {
    using vector = bndvector<T, B>;

    using value_type      = T;
    using key_type        = T;
    using key_compare     = CMP;
    using value_compare   = CMP;
    using difference_type = std::ptrdiff_t;
    using size_type       = typename vector::size_type;
    using index           = typename vector::index;     // of an element, as long as nothing changes

    // The elements can only be read: changing one could break the order
    using CI             = typename vector::CI;
    using I              = CI;
    using iterator       = CI;
    using const_iterator = CI;

    using vector::capacity;
    using vector::clear;
    using vector::empty;
    using vector::full;
    using vector::irange;       // the indices there are right now, in order
    using vector::live;         // is there an element at the index?
    using vector::max_size;
    using vector::size;

    CI begin() const { return vector::cbegin(); }
    CI end() const { return vector::cend(); }
    CI cbegin() const { return vector::cbegin(); }
    CI cend() const { return vector::cend(); }

    // If an equivalent element is already in, returns the iterator to it
    // (and `false`). If there is no room, returns `end()` (and `false`).
    std::pair<CI, bool> insert(T const& v)
    {
        const auto where = put(v, v);
        return { begin() + static_cast<difference_type>(where.first), where.second };
    }
    std::pair<CI, bool> insert(T&& v)
    {
        const auto where = put(v, std::move(v));
        return { begin() + static_cast<difference_type>(where.first), where.second };
    }
    template <class... A> std::pair<CI, bool> emplace(A&&... a) { return insert(T(std::forward<A>(a)...)); }

    // The number of elements erased
    size_type erase(T const& v) { return take_out(v); }
    // Returns the iterator to what followed the erased element, `end()` if
    // there is none or nothing was erased
    CI erase(CI pos) { return vector::erase(pos); }

    size_type count(T const& v) const { return is(first_not_before(v), v); }
    bool      contains(T const& v) const { return is(first_not_before(v), v); }
    CI        find(T const& v) const
    {
        const size_type at = first_not_before(v);
        return is(at, v) ? begin() + static_cast<difference_type>(at) : end();
    }
    // The first element that is not ordered before `v`
    CI lower_bound(T const& v) const { return begin() + static_cast<difference_type>(first_not_before(v)); }
    // The first element that `v` is ordered before
    CI upper_bound(T const& v) const
    {
        const size_type at = first_not_before(v);
        return begin() + static_cast<difference_type>(at + (is(at, v) ? 1 : 0));
    }

    // The index of the element, or nothing
    template <template <class> class V> V<index> index_of(T const& v) const
    {
        const size_type at = first_not_before(v);
        return is(at, v) ? as_index<V>(at) : V<index>{};
    }
    // A copy of the element at the index, or nothing; the smallest and the greatest one
    template <template <class> class V> V<T> at(index i) const { return vector::template at<V>(i); }
    template <template <class> class V> V<T> front() const { return vector::template front<V>(); }
    template <template <class> class V> V<T> back() const { return vector::template back<V>(); }
    // A copy of the element at the index, or `T{}` if there is none
    T get(index i) const { return vector::get(i); }

    // Calls f(T const&) for each element, in order
    template <class F> void for_each(F&& f) const { vector::for_each(std::forward<F>(f)); }

    bool operator==(bndflatset const& x) const { return static_cast<vector const&>(*this) == x; }

protected:
    // What follows is for the views of a flat set (the flat map). `Q` is
    // anything that `CMP` can compare with an element, both ways. Here an
    // element is at its position, which is at most the size.
    using vector::ref;

    // The position of the first element that is not ordered before `v`;
    // the size, if there is none. O(log n).
    template <class Q> size_type first_not_before(Q const& v) const
    {
        CMP       cmp;
        size_type from = 0;
        size_type to   = size();
        while (from < to) {
            const size_type mid = from + (to - from) / 2;
            if (cmp(this->ref(mid), v)) {
                from = mid + 1;
            }
            else {
                to = mid;
            }
        }
        return from;
    }
    // Is there, at `at` (which is where `first_not_before(v)` said), an element equivalent to `v`?
    template <class Q> bool is(size_type at, Q const& v) const { return (at < size()) && !CMP{}(v, this->ref(at)); }

    // Makes an element from `a`, unless there is one equivalent to `key`
    // (which must be how the new element compares), or there is no room.
    // Returns the position of the element; the size (and `false`) if there
    // is no room.
    template <class Q, class... A> std::pair<size_type, bool> put(Q const& key, A&&... a)
    {
        const size_type at = first_not_before(key);
        if (is(at, key)) {
            return { at, false };
        }
        typename vector::position pos;
        if (!pos.be(at) || !vector::emplace(pos, std::forward<A>(a)...)) {
            return { size(), false };
        }
        return { at, true };
    }
    template <class Q> size_type take_out(Q const& v)
    {
        const size_type at = first_not_before(v);
        if (!is(at, v)) {
            return 0;
        }
        vector::erase(begin() + static_cast<difference_type>(at));
        return 1;
    }
    template <template <class> class V> static V<index> as_index(size_type at)
    {
        return index::template be<V>(static_cast<decltype(std::declval<index>().get())>(at));
    }
};

template <class T, int N, class CMP = cobhlp::less<T>> using cobflatset = bndflatset<T, cobbound<N>, CMP>;


#endif // !defined(INC_COBFLATSET)
