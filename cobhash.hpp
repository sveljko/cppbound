/* Compile time bound hash table. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBHASH)
#define      INC_COBHASH


#include "cobarray.hpp"
#include "cobstore.hpp"

#include <cstddef>
#include <functional>
#include <utility>


// Open addressing, linear probing, deletion by backward shift (no
// tombstones). Capacity N entries; it may be filled completely.
template <class K, class V, int N, class H = std::hash<K>, class E = std::equal_to<K>>
struct cobhash {
    static_assert(N > 0, "Hash table must have some capacity");

    using key_type   = K;
    using mapped_type = V;

    struct entry {
        K key;
        V value;
        entry(K const& k, V v) : key(k), value(std::move(v)) {}
    };

    cobhash() { used.fill(false); }
    cobhash(cobhash const& x) : used(x.used), count(x.count) { copy_entries(x); }
    cobhash& operator=(cobhash const& x)
    {
        if (this != &x) {
            clear();
            used  = x.used;
            count = x.count;
            copy_entries(x);
        }
        return *this;
    }
    ~cobhash() { clear(); }

    // false if the key is there, or the table is full
    bool insert(K const& k, V v)
    {
        if ((count == N) || (find_slot(k) >= 0)) {
            return false;
        }
        int s = home_of(k);
        for (int step = 0; step < N; ++step, s = (s + 1) % N) {
            if (!is_used(s)) {
                place(s, k, std::move(v));
                return true;
            }
        }
        return false;
    }
    // false only if the key is absent and the table is full
    bool insert_or_assign(K const& k, V v)
    {
        const int s = find_slot(k);
        if (s >= 0) {
            val_at(s) = std::move(v);
            return true;
        }
        return insert(k, std::move(v));
    }
    bool erase(K const& k)
    {
        int i = find_slot(k);
        if (i < 0) {
            return false;
        }
        drop(i);
        int j = i;
        for (;;) {
            j = (j + 1) % N;
            if (!is_used(j)) {
                break;
            }
            const int home = home_of(key_at(j));
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

    bool contains(K const& k) const { return find_slot(k) >= 0; }
    // A copy of the value, or nothing
    template <template <class> class O> O<V> find(K const& k) const
    {
        const int s = find_slot(k);
        return (s >= 0) ? O<V>(val_at(s)) : O<V>{};
    }
    template <class F> bool with(K const& k, F&& f)
    {
        const int s = find_slot(k);
        if (s >= 0) {
            f(val_at(s));
            return true;
        }
        return false;
    }
    template <class F> bool with(K const& k, F&& f) const
    {
        const int s = find_slot(k);
        if (s >= 0) {
            f(val_at(s));
            return true;
        }
        return false;
    }
    // In slot order
    template <class F> void for_each(F&& f)
    {
        for (int s = 0; s < N; ++s) {
            if (is_used(s)) {
                f(key_at(s), val_at(s));
            }
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (int s = 0; s < N; ++s) {
            if (is_used(s)) {
                f(key_at(s), val_at(s));
            }
        }
    }

    unsigned size() const noexcept { return count; }
    unsigned max_size() const noexcept { return N; }
    bool     empty() const noexcept { return count == 0; }
    bool     full() const noexcept { return count == N; }

    void clear()
    {
        for (int s = 0; s < N; ++s) {
            if (is_used(s)) {
                drop(s);
            }
        }
    }

private:
    using index = cobint<0, N - 1>;

    static int home_of(K const& k)
    {
        return static_cast<int>(H{}(k) % static_cast<std::size_t>(N));
    }
    bool is_used(int s) const
    {
        index i;
        return i.be(s) && used.get(i);
    }
    K const& key_at(int s) const { index i; i.be(s); return store.at(i).key; }
    V&       val_at(int s) { index i; i.be(s); return store.at(i).value; }
    V const& val_at(int s) const { index i; i.be(s); return store.at(i).value; }

    // From the home slot, at most N steps
    int find_slot(K const& k) const
    {
        int s = home_of(k);
        for (int step = 0; step < N; ++step, s = (s + 1) % N) {
            if (!is_used(s)) {
                return -1;
            }
            if (E{}(key_at(s), k)) {
                return s;
            }
        }
        return -1;
    }

    void place(int s, K const& k, V v)
    {
        index i;
        if (i.be(s)) {
            store.make(i, k, std::move(v));
            used.set(i, true);
            ++count;
        }
    }
    // Destroys the entry, `used[s] = false`, `--count`
    void drop(int s)
    {
        index i;
        if (i.be(s)) {
            store.drop(i);
            used.set(i, false);
            --count;
        }
    }
    // Constructs at `to` from `from`, destroys `from`
    void move_entry(int from, int to)
    {
        index f, t;
        if (f.be(from) && t.be(to)) {
            store.make(t, std::move(store.at(f)));
            store.drop(f);
            used.set(t, true);
            used.set(f, false);
        }
    }
    void copy_entries(cobhash const& x)
    {
        for (int s = 0; s < N; ++s) {
            index i;
            if (i.be(s) && used.get(i)) {
                store.make(i, x.store.at(i));
            }
        }
    }

    cobstore<entry, N> store;
    cobarray<bool, N>  used;
    unsigned           count = 0;
};


#endif // !defined(INC_COBHASH)
