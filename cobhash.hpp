/* Compile time bound hash table. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBHASH)
#define      INC_COBHASH


#include "cobbound.hpp"

#include <cstddef>
#include <functional>
#include <utility>


// Open addressing, linear probing, deletion by backward shift (no
// tombstones). It may be filled completely.
template <class K, class V, class B, class H = std::hash<K>, class E = std::equal_to<K>>
struct bndhash {
    using key_type    = K;
    using mapped_type = V;
    using size_type   = typename B::size_type;

    struct entry {
        K key;
        V value;
        entry(K const& k, V v) : key(k), value(std::move(v)) {}
    };

    bndhash() { used.fill(false); }
    bndhash(bndhash const& x) : used(x.used), count(x.count) { copy_entries(x); }
    bndhash& operator=(bndhash const& x)
    {
        if (this != &x) {
            clear();
            used  = x.used;
            count = x.count;
            copy_entries(x);
        }
        return *this;
    }
    ~bndhash() { clear(); }

    // false if the key is there, or the table is full
    bool insert(K const& k, V v)
    {
        if (full() || (find_slot(k) != none())) {
            return false;
        }
        size_type s = home_of(k);
        while (is_used(s)) {    // not full, so there is a slot
            s = after(s);
        }
        place(s, k, std::move(v));
        return true;
    }
    // false only if the key is absent and the table is full
    bool insert_or_assign(K const& k, V v)
    {
        const size_type s = find_slot(k);
        if (s != none()) {
            val_at(s) = std::move(v);
            return true;
        }
        return insert(k, std::move(v));
    }
    bool erase(K const& k)
    {
        size_type i = find_slot(k);
        if (i == none()) {
            return false;
        }
        drop(i);
        size_type j = i;
        for (;;) {
            j = after(j);
            if (!is_used(j)) {
                break;
            }
            const size_type home = home_of(key_at(j));
            // Leave the entry at j alone if its home lies cyclically in (i, j]
            const bool stays = (i <= j) ? ((i < home) && (home <= j))
                                        : ((i < home) || (home <= j));
            if (stays) {
                continue;
            }
            move_entry(j, i);
            i = j;
        }
        return true;
    }

    bool contains(K const& k) const { return find_slot(k) != none(); }
    // A copy of the value, or nothing
    template <template <class> class O> O<V> find(K const& k) const
    {
        const size_type s = find_slot(k);
        return (s != none()) ? O<V>(val_at(s)) : O<V>{};
    }
    template <class F> bool with(K const& k, F&& f)
    {
        const size_type s = find_slot(k);
        if (s != none()) {
            f(val_at(s));
            return true;
        }
        return false;
    }
    template <class F> bool with(K const& k, F&& f) const
    {
        const size_type s = find_slot(k);
        if (s != none()) {
            f(val_at(s));
            return true;
        }
        return false;
    }
    // In slot order
    template <class F> void for_each(F&& f)
    {
        for (auto i : B::slots()) {
            if (used.get(i)) {
                f(store.at(i).key, store.at(i).value);
            }
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (auto i : B::slots()) {
            if (used.get(i)) {
                f(store.at(i).key, store.at(i).value);
            }
        }
    }

    size_type size() const noexcept { return count; }
    size_type max_size() const noexcept { return B::size(); }
    bool      empty() const noexcept { return count == 0; }
    bool      full() const noexcept { return count == B::size(); }

    void clear()
    {
        for (auto i : B::slots()) {
            if (used.get(i)) {
                store.drop(i);
                used.set(i, false);
            }
        }
        count = 0;
    }

private:
    using index = typename B::slot;

    // Slots are numbered from 0; this number stands for "no slot"
    static size_type none() { return B::size(); }
    // Only called when there is some capacity
    static size_type home_of(K const& k)
    {
        return static_cast<size_type>(H{}(k) % static_cast<std::size_t>(B::size()));
    }
    static size_type after(size_type s) { return (s + 1 < B::size()) ? s + 1 : 0; }

    bool is_used(size_type s) const
    {
        index i;
        return i.be(s) && used.get(i);
    }
    K const& key_at(size_type s) const { index i; i.be(s); return store.at(i).key; }
    V&       val_at(size_type s) { index i; i.be(s); return store.at(i).value; }
    V const& val_at(size_type s) const { index i; i.be(s); return store.at(i).value; }

    // From the home slot, at most as many steps as there are slots
    size_type find_slot(K const& k) const
    {
        if (count == 0) {
            return none();
        }
        size_type s = home_of(k);
        for (size_type step = 0; step < B::size(); ++step, s = after(s)) {
            if (!is_used(s)) {
                return none();
            }
            if (E{}(key_at(s), k)) {
                return s;
            }
        }
        return none();
    }

    void place(size_type s, K const& k, V v)
    {
        index i;
        if (i.be(s)) {
            store.make(i, k, std::move(v));
            used.set(i, true);
            ++count;
        }
    }
    // Destroys the entry, `used[s] = false`, `--count`
    void drop(size_type s)
    {
        index i;
        if (i.be(s)) {
            store.drop(i);
            used.set(i, false);
            --count;
        }
    }
    // Constructs at `to` from `from`, destroys `from`
    void move_entry(size_type from, size_type to)
    {
        index f, t;
        if (f.be(from) && t.be(to)) {
            store.make(t, std::move(store.at(f)));
            store.drop(f);
            used.set(t, true);
            used.set(f, false);
        }
    }
    void copy_entries(bndhash const& x)
    {
        for (auto i : B::slots()) {
            if (used.get(i)) {
                store.make(i, x.store.at(i));
            }
        }
    }

    typename B::template store<entry> store;
    typename B::template array<bool>  used;
    size_type                         count = 0;
};

template <class K, class V, int N, class H = std::hash<K>, class E = std::equal_to<K>>
using cobhash = bndhash<K, V, cobbound<N>, H, E>;


#endif // !defined(INC_COBHASH)
