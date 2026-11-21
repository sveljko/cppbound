# Hash table

```cpp
#include "cobhash.hpp"      // or "atbhash.hpp"

cobhash<K, V, N, H, E>      atbhash<K, V, Tag, H, E>
```

At most `N` keys, each with a value, found by the hash of the key.
`H` is the hash (by default `std::hash<K>`) and `E` tells whether two
keys are equal (by default `std::equal_to<K>`). It is what
`std::unordered_map` is for, and this document says how it differs
from that one - which is a lot, as the interface is cut down to what
is safe and needed.

Read [common.md](common.md) for what is said there about capacity and
the generic optional (here its template parameter is called `O`).
There are no iterators, no links, and no checked or hive variants.


# How it works, and how fast it is

Open addressing with linear probing: the keys and values are in one
array of `N` slots, a key is looked for from the slot its hash points
to onwards, and erasing moves the keys that follow back into place
(so there are no "tombstones", and erasing does not slow down what
comes after).

The table can be filled completely. But the fuller it is, the longer
the runs of slots that are looked at: each operation takes a few
steps on average while the table is, say, less than three quarters
full, and up to `N` steps when it is nearly full or when many keys
hash alike. If you need the bound, make `N` larger than the number
of keys you expect - or use `cobtreemap` ([cobmap.md](cobmap.md)).

Nothing is ever rehashed, and the keys stay in their slots unless a
key before them in their run is erased.

`H` and `E` are default constructed whenever they are needed, so they
can't have a state.


# What is not there

Of `std::unordered_map`, these are missing:

* Iterators, and all that takes or returns them. `find()` returns a
  copy of the value; going over the table is done with `for_each()`.
* `operator[]` and `at()`. To read a value, use `find<O>(key)`; to
  change it, `with(key, f)`.
* `emplace()`, `try_emplace()`, hints, and `insert()` of a pair, a
  range or an initializer list.
* `count()` and `equal_range()`. (`contains()` is there.)
* `swap()`, `merge()`, `extract()` and node handles.
* Buckets, the load factor, `rehash()` and `reserve()`.
* `hash_function()` and `key_eq()`.
* Constructors other than the default and the copy constructor,
  allocators, comparison of tables, move operations (a "move"
  copies), and lookup by a type other than `K`.


# What is different


## Putting in

```cpp
bool insert(K const& k, V v);
bool insert_or_assign(K const& k, V v);
```

The key and the value are given apart, and the result is a `bool`.

`insert()` returns `true` if the key was put in. It returns `false`,
and changes nothing, if the key is in the table already, or if the
table is full - it does not tell which of the two it was; ask
`contains()` or `full()` if that matters.

`insert_or_assign()` puts the key in, or assigns `v` to its value if
it is in already, and returns `true`. It returns `false` only if the
key is not in and the table is full.

The value is taken by value and moved in.


## Finding

```cpp
template <template <class> class O> O<V> find(K const& k) const;
bool contains(K const& k) const;
```

`find<O>(k)` returns a copy of the value in a generic optional, or
nothing if the key is not in.

```cpp
cobhash<std::string, int, 64> h;
h.insert("one", 1);
auto one = h.find<std::optional>("one");    // 1
```


## Erasing

```cpp
bool erase(K const& k);
```

Returns whether the key was in (and so was erased), rather than a
count.


# What is added

```cpp
template <class F> bool with(K const& k, F&& f);
template <class F> bool with(K const& k, F&& f) const;
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
bool full() const noexcept;
```

`with()` calls `f` with a reference to the value of the key and
returns `true`. If the key is not in, `f` is not called and `false`
is returned.

`for_each()` calls `f(key, value)` for each key, in the order of the
slots - which is no order to speak of, and changes as keys are put in
and erased. It looks at all `N` slots. The value can be changed
(unless the table is `const`). The key must not be: it is where it is
because of what it is.

`full()` tells whether all the slots are in use.


# What it takes

For each of the `N` slots: room for a `K` and a `V`, and a byte that
tells whether the slot is in use.
