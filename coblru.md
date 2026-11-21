# Least recently used cache

```cpp
#include "coblru.hpp"       // or "atblru.hpp"

coblru<K, V, N, H, E>       atblru<K, V, Tag, H, E>
cobhivelru<K, V, N, H, E>   atbhivelru<K, V, Tag, H, E>
```

At most `N` keys, each with a value. When a new key is put in and
there is no room, the key that was not used for the longest time is
dropped to make room. So putting a key in never fails for the lack of
room: the cache holds the `N` keys that were used most recently.

`H` is the hash of a key (by default `std::hash<K>`) and `E` tells
whether two keys are equal (by default `std::equal_to<K>`). Both are
default constructed whenever they are needed, so they can't have a
state.

There is no Standard counterpart.

See [common.md](common.md) for the generic optional (here its
template parameter is called `O`) and for what the `hive` variant
is. There is no checked variant, as the cache gives out no links.
There are no iterators.


# What is a use

Each key is either used by a call or not, and that is what decides
which key is dropped next:

| A use of the key         | Not a use                  |
|--------------------------|----------------------------|
| `put()`                  | `peek()`                   |
| `get()`                  | `contains()`               |
| `with()`                 | `for_each()`               |
|                          | `eldest()`                 |

A use makes the key the most recently used one. A use that finds no
such key changes nothing.


# How fast it is

It is a hash table (with chaining) and a list of the keys by how
recently they were used, both kept in one pool
([cobpool.md](cobpool.md)).

Each operation takes a few steps on average. As in any hash table, if
many keys hash alike, an operation takes as many steps as there are
such keys: up to `N`. Dropping the least recently used key, and
making a key the most recently used one, are O(1).


# Put a value in

```cpp
template <class U = V> bool put(K const& k, U&& v);
template <class U, class F> bool put(K const& k, U&& v, F&& gone);
```

If the key is in the cache, assigns `v` to its value. Otherwise puts
the key in, with a value constructed from `v`. Either way, this is a
use of the key.

If the key is not in and the cache is full, the least recently used
key is dropped first. Just before it is, `gone(key, value)` is called
with it, if `gone` was given, so that what is being dropped can be
seen to - saved, closed, counted:

```cpp
coblru<std::string, page, 16> cache;
cache.put(name, load(name), [](std::string const& n, page& p) { save(n, p); });
```

`gone` is not called when the value of a key that is in is replaced,
nor by `erase()` or `clear()`.

Returns `true`. It returns `false` only if the cache has no capacity
at all, which can only be so for an `atblru` whose dimension is 0 or
was not set.

`k` and `v` must not refer to a key or a value that is in the cache:
it might be the one that is dropped.


# Get a value

```cpp
template <template <class> class O> O<V> get(K const& k);
template <template <class> class O> O<V> peek(K const& k) const;
```

Both return a copy of the value of the key in a generic optional, or
nothing if the key is not in the cache.

`get()` is a use of the key. `peek()` is not: it leaves the order of
use as it is.

```cpp
auto p = cache.get<std::optional>(name);
if (!p) {
    cache.put(name, load(name));
}
```


# Change a value where it is

```cpp
template <class F> bool with(K const& k, F&& f);
```

Calls `f` with a reference to the value of the key and returns
`true`; this is a use of the key. If the key is not in, `f` is not
called and `false` is returned.


# Is a key in

```cpp
bool contains(K const& k) const;
```

Whether the key is in the cache. Not a use.


# Take a key out

```cpp
bool erase(K const& k);
```

Takes the key and its value out. Returns `false` if the key was not
in.


# The key that would go next

```cpp
template <template <class> class O> O<K> eldest() const;
```

Returns a copy of the least recently used key in a generic optional -
the one that the next `put()` of a new key would drop, if the cache
were full. Returns nothing if the cache is empty.


# Go over all of it

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
```

Calls `f(key, value)` for each key, the most recently used one first
and the least recently used one last. The key is a reference to
`const`; the value can be changed, unless the cache is `const`.

This is not a use of any key.


# Size and clearing

```cpp
size_type size() const;
size_type max_size() const;
bool      empty() const;
bool      full() const;
void      clear();
```

`clear()` takes everything out, without calling any `gone`.


# What it takes

For each of the `N` slots: room for a `K` and a `V`, four links (an
`int` each for `coblru`), and a byte - or, for `cobhivelru`, a bit -
that tells whether the slot is in use.
