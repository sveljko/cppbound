/* Compile time bound map. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBMAP)
#define      INC_COBMAP


#include "cobskiplist.hpp"

#include <utility>


namespace cobhlp {

// Orders what a map keeps, and compares it with a key, by `CMP` of the keys
template <class K, class V, class CMP> struct bykey {
    bool operator()(keyval<K, V> const& x, keyval<K, V> const& y) const { return CMP{}(x.key, y.key); }
    bool operator()(keyval<K, V> const& x, K const& y) const { return CMP{}(x.key, y); }
    bool operator()(K const& x, keyval<K, V> const& y) const { return CMP{}(x, y.key); }
};

} // namespace cobhlp


// A key-value view of an ordered set `S` of `{ key, value }` (the skiplist,
// or the tree): the elements are ordered by their keys, each key is there
// at most once, and the value that goes with it can be changed. Iterators
// and `for_each` go in the order of the keys.
template <class K, class V, class S>
struct bndmapview : private S {
    using skiplist = S;

    using key_type    = K;
    using mapped_type = V;
    using value_type  = cobhlp::keyval<K, V>;
    using size_type   = typename skiplist::size_type;
    using link        = typename skiplist::link;

    using typename skiplist::CI;
    using typename skiplist::const_iterator;
    using typename skiplist::iterator;

    using skiplist::nil;

    // Of the set: these do not depend on what is a key
    using skiplist::begin;      // the iterators give `{ key, value }`, read only
    using skiplist::cbegin;
    using skiplist::cend;
    using skiplist::clear;
    using skiplist::empty;
    using skiplist::end;
    using skiplist::full;
    using skiplist::fwd;
    using skiplist::lbegin;
    using skiplist::lend;
    using skiplist::lfront;
    using skiplist::live;
    using skiplist::max_size;
    using skiplist::size;

    // Makes the value from `a`. If the key is already in, returns the link to
    // it (and `false`). If there is no room, returns `nil` (and `false`).
    template <class... A> std::pair<link, bool> emplace(K const& k, A&&... a)
    {
        return this->put(k, k, std::forward<A>(a)...);
    }
    std::pair<link, bool> insert(K const& k, V const& v) { return emplace(k, v); }
    std::pair<link, bool> insert(K const& k, V&& v) { return emplace(k, std::move(v)); }
    // `nil` (and `false`) only if the key is absent and there is no room
    template <class U = V> std::pair<link, bool> insert_or_assign(K const& k, U&& v)
    {
        const link l = at_key(k);
        if (value_type* e = entry(l)) {
            e->value = std::forward<U>(v);
            return { l, false };
        }
        return emplace(k, std::forward<U>(v));
    }

    // The number of elements erased
    size_type erase(K const& k) { return this->unlink(k, nil); }
    // Returns the link following the erased one, `nil` if there is none or
    // nothing was erased
    link erase(link pos) { return skiplist::erase(pos); }
    CI   erase(CI pos) { return skiplist::erase(pos); }

    bool      contains(K const& k) const { return at_key(k) != nil; }
    bool      contains(link l) const { return live(l); }
    bool      contains(CI pos) const { return skiplist::contains(pos); }
    size_type count(K const& k) const { return contains(k); }
    CI        find(K const& k) const { return this->iter_at(at_key(k)); }
    // The link of the key, `nil` if it is not in
    link where(K const& k) const { return at_key(k); }
    // The link of the first key that is not ordered before `k`
    link lower_bound(K const& k) const { return this->lower(k); }
    // The link of the first key that `k` is ordered before
    link upper_bound(K const& k) const { return this->upper(k); }

    // A copy of the value, or nothing
    template <template <class> class O> O<V> find(K const& k) const { return value<O>(at_key(k)); }
    // A copy of the value at the link, or nothing
    template <template <class> class O> O<V> value(link l) const
    {
        value_type const* e = entry(l);
        return (e != nullptr) ? O<V>(e->value) : O<V>{};
    }
    // A copy of the key at the link, or nothing
    template <template <class> class O> O<K> key(link l) const
    {
        value_type const* e = entry(l);
        return (e != nullptr) ? O<K>(e->key) : O<K>{};
    }

    // Calls f with the value of the key, if it is in. Returns whether it did.
    template <class F> bool with(K const& k, F&& f) { return with(at_key(k), std::forward<F>(f)); }
    template <class F> bool with(K const& k, F&& f) const { return with(at_key(k), std::forward<F>(f)); }
    template <class F> bool with(link l, F&& f)
    {
        if (value_type* e = entry(l)) {
            f(e->value);
            return true;
        }
        return false;
    }
    template <class F> bool with(link l, F&& f) const
    {
        if (value_type const* e = entry(l)) {
            f(e->value);
            return true;
        }
        return false;
    }

    // Calls f(key, value) for each element, in the order of the keys
    template <class F> void for_each(F&& f)
    {
        for (link l = lbegin(); l != nil; l = fwd(l)) {
            if (auto* e = entry(l)) {
                f(e->key, e->value);
            }
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (link l = lbegin(); l != nil; l = fwd(l)) {
            if (auto* e = entry(l)) {
                f(e->key, e->value);
            }
        }
    }

private:
    // What is kept at the link, if anything is
    value_type* entry(link l)
    {
        value_type* e = nullptr;
        skiplist::with(l, [&](value_type& x) { e = &x; });
        return e;
    }
    value_type const* entry(link l) const
    {
        value_type const* e = nullptr;
        skiplist::with(l, [&](value_type const& x) { e = &x; });
        return e;
    }

    link at_key(K const& k) const
    {
        const link l = this->lower(k);
        return this->holds(l, k) ? l : link{ nil };
    }
};

// The map that is a view of the skiplist
template <class K, class V, class B, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32, bool CHECKED = false,
          bool HIVE = false>
using bndmap = bndmapview<K, V, bndskiplist<cobhlp::keyval<K, V>, B, cobhlp::bykey<K, V, CMP>, RNG, CHECKED, HIVE>>;

template <class K, class V, int N, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using cobmap = bndmap<K, V, cobbound<N>, CMP, RNG>;

// The one with checked links
template <class K, class V, int N, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using cobcheckedmap = bndmap<K, V, cobbound<N>, CMP, RNG, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class K, class V, int N, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using cobhivemap = bndmap<K, V, cobbound<N>, CMP, RNG, false, true>;
template <class K, class V, int N, class CMP = cobhlp::less<K>, class RNG = cobhlp::xorshift32>
using cobcheckedhivemap = bndmap<K, V, cobbound<N>, CMP, RNG, true, true>;


#endif // !defined(INC_COBMAP)
