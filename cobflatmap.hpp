/* Compile time bound flat map. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBFLATMAP)
#define      INC_COBFLATMAP


#include "cobflatset.hpp"

#include <utility>


namespace cobhlp {

// What a flat map keeps for a key. It has to be such that it can be moved
// about, so the key is kept from being changed by not giving it out as
// anything but constant.
template <class K, class V> struct flatkeyval {
    K key;
    V value;

    flatkeyval() : key(), value() {}
    template <class... A> explicit flatkeyval(K const& k, A&&... a) : key(k), value(std::forward<A>(a)...) {}
};

// Orders what a flat map keeps, and compares it with a key, by `CMP` of the keys
template <class K, class V, class CMP> struct flatbykey {
    bool operator()(flatkeyval<K, V> const& x, flatkeyval<K, V> const& y) const { return CMP{}(x.key, y.key); }
    bool operator()(flatkeyval<K, V> const& x, K const& y) const { return CMP{}(x.key, y); }
    bool operator()(K const& x, flatkeyval<K, V> const& y) const { return CMP{}(x, y.key); }
};

} // namespace cobhlp


// Keys with their values, kept in the order of the keys in a vector: a
// key-value view of the flat set, with all that goes for that one. The
// value of a key can be changed, the key can not.
template <class K, class V, class B, class CMP = cobhlp::less<K>>
struct bndflatmap : private bndflatset<cobhlp::flatkeyval<K, V>, B, cobhlp::flatbykey<K, V, CMP>> {
    using set = bndflatset<cobhlp::flatkeyval<K, V>, B, cobhlp::flatbykey<K, V, CMP>>;

    using key_type        = K;
    using mapped_type     = V;
    using value_type      = cobhlp::flatkeyval<K, V>;
    using key_compare     = CMP;
    using difference_type = std::ptrdiff_t;
    using size_type       = typename set::size_type;
    using index           = typename set::index;

    using typename set::CI;
    using typename set::const_iterator;
    using typename set::iterator;

    // Of the set: these do not depend on what is a key
    using set::begin;       // the iterators give `{ key, value }`, read only
    using set::capacity;
    using set::cbegin;
    using set::cend;
    using set::clear;
    using set::empty;
    using set::end;
    using set::full;
    using set::irange;
    using set::live;
    using set::max_size;
    using set::size;

    // Makes the value from `a`. If the key is already in, returns the
    // iterator to it (and `false`). If there is no room, returns `end()`
    // (and `false`).
    template <class... A> std::pair<CI, bool> emplace(K const& k, A&&... a)
    {
        const auto where = this->put(k, k, std::forward<A>(a)...);
        return { begin() + static_cast<difference_type>(where.first), where.second };
    }
    std::pair<CI, bool> insert(K const& k, V const& v) { return emplace(k, v); }
    std::pair<CI, bool> insert(K const& k, V&& v) { return emplace(k, std::move(v)); }
    // `end()` (and `false`) only if the key is absent and there is no room
    template <class U = V> std::pair<CI, bool> insert_or_assign(K const& k, U&& v)
    {
        const size_type at = this->first_not_before(k);
        if (this->is(at, k)) {
            this->ref(at).value = std::forward<U>(v);
            return { begin() + static_cast<difference_type>(at), false };
        }
        return emplace(k, std::forward<U>(v));
    }

    // The number of elements erased
    size_type erase(K const& k) { return this->take_out(k); }
    // Returns the iterator to what followed the erased element, `end()` if
    // there is none or nothing was erased
    CI erase(CI pos) { return set::erase(pos); }

    bool      contains(K const& k) const { return this->is(this->first_not_before(k), k); }
    size_type count(K const& k) const { return contains(k); }
    CI        find(K const& k) const
    {
        const size_type at = this->first_not_before(k);
        return this->is(at, k) ? begin() + static_cast<difference_type>(at) : end();
    }
    // The first key that is not ordered before `k`
    CI lower_bound(K const& k) const { return begin() + static_cast<difference_type>(this->first_not_before(k)); }
    // The first key that `k` is ordered before
    CI upper_bound(K const& k) const
    {
        const size_type at = this->first_not_before(k);
        return begin() + static_cast<difference_type>(at + (this->is(at, k) ? 1 : 0));
    }

    // A copy of the value, or nothing
    template <template <class> class O> O<V> find(K const& k) const
    {
        const size_type at = this->first_not_before(k);
        return this->is(at, k) ? O<V>(this->ref(at).value) : O<V>{};
    }
    // The index of the key, or nothing
    template <template <class> class O> O<index> index_of(K const& k) const
    {
        const size_type at = this->first_not_before(k);
        return this->is(at, k) ? set::template as_index<O>(at) : O<index>{};
    }
    // A copy of the key and of the value at the index, or nothing
    template <template <class> class O> O<K> key(index i) const
    {
        return live(i) ? O<K>(this->ref(place(i)).key) : O<K>{};
    }
    template <template <class> class O> O<V> value(index i) const
    {
        return live(i) ? O<V>(this->ref(place(i)).value) : O<V>{};
    }

    // Calls f with the value of the key, if it is in. Returns whether it did.
    template <class F> bool with(K const& k, F&& f)
    {
        const size_type at = this->first_not_before(k);
        if (!this->is(at, k)) {
            return false;
        }
        f(this->ref(at).value);
        return true;
    }
    template <class F> bool with(K const& k, F&& f) const
    {
        const size_type at = this->first_not_before(k);
        if (!this->is(at, k)) {
            return false;
        }
        f(this->ref(at).value);
        return true;
    }
    // The same, for the value at the index
    template <class F> bool with(index i, F&& f)
    {
        if (!live(i)) {
            return false;
        }
        f(this->ref(place(i)).value);
        return true;
    }

    // Calls f(key, value) for each element, in the order of the keys
    template <class F> void for_each(F&& f)
    {
        for (size_type at = 0; at < size(); ++at) {
            f(std::as_const(this->ref(at).key), this->ref(at).value);
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (size_type at = 0; at < size(); ++at) {
            f(this->ref(at).key, this->ref(at).value);
        }
    }

private:
    static size_type place(index i) { return static_cast<size_type>(i.get()); }
};

template <class K, class V, int N, class CMP = cobhlp::less<K>>
using cobflatmap = bndflatmap<K, V, cobbound<N>, CMP>;


#endif // !defined(INC_COBFLATMAP)
