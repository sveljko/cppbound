/* Compile time bound least-recently-used cache. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBLRU)
#define      INC_COBLRU


#include "cobpool.hpp"

#include <cstddef>
#include <functional>
#include <utility>


// Keys and their values, as many as there is room for: when a new key is
// put in and there is no room, the one that was not used for the longest
// time is dropped. Putting, getting and changing a value is "using" it.
//
// A hash table (with chaining) and a list of the keys by how recently they
// were used, both in the same pool. Each operation takes a few steps on
// average; if many keys hash alike, as many steps as there are such keys.
// With `HIVE` being `true`, which slots hold an element is kept as a bit each
// (see `cobhive`): an eighth of the room for that, a little more to ask.
template <class K, class V, class B, class H = std::hash<K>, class E = std::equal_to<K>, bool HIVE = false>
struct bndlru : protected bndpool<cobhlp::keyval<K, V>, B, false, HIVE> {
    using pool        = bndpool<cobhlp::keyval<K, V>, B, false, HIVE>;
    using key_type    = K;
    using mapped_type = V;
    using size_type   = typename B::size_type;

    bndlru() : newest(nil), oldest(nil)
    {
        bucket.fill(nil);
        chain.fill(nil);
        newer.fill(nil);
    }

    using pool::empty;
    using pool::full;
    using pool::max_size;
    using pool::size;

    // Puts the value in, or changes it if the key is there; either way, it
    // is now the most recently used one. If there is no room, the least
    // recently used one is dropped first, after `gone(key, value)` is called
    // with it, if given. false only if there is no room at all.
    template <class U = V> bool put(K const& k, U&& v)
    {
        return put(k, std::forward<U>(v), [](K const&, V&) {});
    }
    template <class U, class F> bool put(K const& k, U&& v, F&& gone)
    {
        const link found = find(k);
        index      i;
        if (i.be(found.get())) {
            this->at(i).value = std::forward<U>(v);
            touch(found);
            return true;
        }
        if (B::size() == 0) {
            return false;
        }
        if (full() && i.be(oldest.get())) {
            gone(this->at(i).key, this->at(i).value);
            remove(oldest);
        }
        const link fresh = this->make(k, std::forward<U>(v));
        if (!i.be(fresh.get())) {
            return false;
        }
        const size_type home = home_of(k);
        chain.set(i, head_of(home));
        set_head(home, fresh);
        attach(fresh);
        return true;
    }

    // A copy of the value, or nothing. This is a use of it.
    template <template <class> class O> O<V> get(K const& k)
    {
        const link found = find(k);
        index      i;
        if (!i.be(found.get())) {
            return {};
        }
        touch(found);
        return O<V>(this->at(i).value);
    }
    // The same, but this is not a use
    template <template <class> class O> O<V> peek(K const& k) const
    {
        index i;
        return i.be(find(k).get()) ? O<V>(this->at(i).value) : O<V>{};
    }
    // Calls f with the value of the key, if it is in. Returns whether it
    // did. This is a use of it.
    template <class F> bool with(K const& k, F&& f)
    {
        const link found = find(k);
        index      i;
        if (!i.be(found.get())) {
            return false;
        }
        touch(found);
        f(this->at(i).value);
        return true;
    }
    bool contains(K const& k) const { return find(k) != nil; }

    bool erase(K const& k)
    {
        const link found = find(k);
        if (found == nil) {
            return false;
        }
        remove(found);
        return true;
    }
    // The key that would be dropped next, or nothing
    template <template <class> class O> O<K> eldest() const
    {
        index i;
        return i.be(oldest.get()) ? O<K>(this->at(i).key) : O<K>{};
    }
    // Calls f(key, value) for each one, the most recently used one first.
    // This is not a use.
    template <class F> void for_each(F&& f)
    {
        index i;
        for (link l = newest; i.be(l.get()); l = this->link_of(i)) {
            f(std::as_const(this->at(i).key), this->at(i).value);
        }
    }
    template <class F> void for_each(F&& f) const
    {
        index i;
        for (link l = newest; i.be(l.get()); l = this->link_of(i)) {
            f(this->at(i).key, this->at(i).value);
        }
    }

    void clear()
    {
        pool::clear();
        bucket.fill(nil);
        newest = oldest = nil;
    }

private:
    using link  = typename B::link;
    using index = typename B::slot;

    static constexpr auto nil = B::nil;

    // Only called when there is some capacity
    static size_type home_of(K const& k)
    {
        return static_cast<size_type>(H{}(k) % static_cast<std::size_t>(B::size()));
    }
    link head_of(size_type home) const
    {
        index i;
        return i.be(home) ? bucket.get(i) : link{ nil };
    }
    void set_head(size_type home, link l)
    {
        index i;
        if (i.be(home)) {
            bucket.set(i, l);
        }
    }
    link find(K const& k) const
    {
        if (empty()) {
            return nil;
        }
        index i;
        for (link l = head_of(home_of(k)); i.be(l.get()); l = chain.get(i)) {
            if (E{}(this->at(i).key, k)) {
                return l;
            }
        }
        return nil;
    }

    // The list of use: from `newest`, by the link that the pool gives to
    // each object, to `oldest`; `newer` goes the other way.
    void attach(link l)
    {
        index i;
        if (!i.be(l.get())) {
            return;
        }
        this->link_to(i, newest);
        newer.set(i, nil);
        index was;
        if (was.be(newest.get())) {
            newer.set(was, l);
        }
        else {
            oldest = l;
        }
        newest = l;
    }
    void detach(link l)
    {
        index i;
        if (!i.be(l.get())) {
            return;
        }
        const link after  = this->link_of(i);     // used before it
        const link before = newer.get(i);         // used after it
        index      x;
        if (x.be(before.get())) { this->link_to(x, after); } else { newest = after; }
        if (x.be(after.get())) { newer.set(x, before); } else { oldest = before; }
    }
    void touch(link l)
    {
        if (l != newest) {
            detach(l);
            attach(l);
        }
    }
    // Out of both the table and the list, and destroyed
    void remove(link gone)
    {
        index i;
        if (!i.be(gone.get())) {
            return;
        }
        const size_type home = home_of(this->at(i).key);
        if (head_of(home) == gone) {
            set_head(home, chain.get(i));
        }
        else {
            index x;
            for (link l = head_of(home); x.be(l.get()); l = chain.get(x)) {
                if (chain.get(x) == gone) {
                    chain.set(x, chain.get(i));
                    break;
                }
            }
        }
        detach(gone);
        this->drop(gone);
    }

    link                             newest;
    link                             oldest;
    typename B::template array<link> bucket;    // the first key of each hash value
    typename B::template array<link> chain;     // the next key of the same hash value
    typename B::template array<link> newer;     // the key used right after this one
};

template <class K, class V, int N, class H = std::hash<K>, class E = std::equal_to<K>>
using coblru = bndlru<K, V, cobbound<N>, H, E>;

// The one where which slots hold an entry is kept as a bit each (see `cobhive`)
template <class K, class V, int N, class H = std::hash<K>, class E = std::equal_to<K>>
using cobhivelru = bndlru<K, V, cobbound<N>, H, E, true>;


#endif // !defined(INC_COBLRU)
