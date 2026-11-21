# Ordered maps

```cpp
#include "cobmap.hpp"           // or "atbmap.hpp"

cobmap<K, V, N, CMP, RNG>               atbmap<K, V, Tag, CMP, RNG>
cobcheckedmap<K, V, N, CMP, RNG>        atbcheckedmap<K, V, Tag, CMP, RNG>
cobhivemap<K, V, N, CMP, RNG>           atbhivemap<K, V, Tag, CMP, RNG>
cobcheckedhivemap<K, V, N, CMP, RNG>    atbcheckedhivemap<K, V, Tag, CMP, RNG>

#include "cobtreemap.hpp"       // or "atbtreemap.hpp"

cobtreemap<K, V, N, CMP>                atbtreemap<K, V, Tag, CMP>
cobcheckedtreemap<K, V, N, CMP>         atbcheckedtreemap<K, V, Tag, CMP>
cobhivetreemap<K, V, N, CMP>            atbhivetreemap<K, V, Tag, CMP>
cobcheckedhivetreemap<K, V, N, CMP>     atbcheckedhivetreemap<K, V, Tag, CMP>
```

At most `N` keys, each with a value, ordered by the keys with `CMP`
(by default `cobhlp::less<K>`), with an interface like that of
`std::map`. This document says how they differ from `std::map`.

`cobmap` is a view of the skiplist and `cobtreemap` a view of the
tree, so everything in [cobskiplist.md](cobskiplist.md) about which
one to choose, how fast they are, the random generator `RNG` and the
room they take goes for the maps too. In short: `cobtreemap` where
the time of each operation has to be bound, `cobmap` otherwise.

Read [common.md](common.md) first: what it says about capacity, the
generic optional (here its template parameter is called `O`, as `V`
is the type of the values), links, checked links, the hive option and
iterators all goes for the maps.

A map differs from `std::map` more than the other containers differ
from their counterparts, because `std::map` is all about references
to values, and these maps do not give any out.


# What is kept

```cpp
using key_type    = K;
using mapped_type = V;
using value_type  = cobhlp::keyval<K, V>;

struct keyval {
    K const key;
    V       value;
};
```

What is kept for a key is not a `std::pair`, but a struct with the
members `key` and `value`. That is what an iterator refers to:

```cpp
for (auto const& e : map) {
    use(e.key, e.value);
}
```


# What is not there

Of `std::map`, these are missing:

* `operator[]` and `at()`. To read a value, use `find<O>(key)`; to
  change it, `with(key, f)`; to put one in, `insert_or_assign()`.
* `try_emplace()`: `emplace()` is what `try_emplace()` is in the
  Standard, see below.
* `equal_range()`, and `lower_bound()` and `upper_bound()` that
  return iterators (they return links).
* `insert()` of a `value_type`, of a range or an initializer list,
  and everything that takes a hint.
* `erase()` of a range.
* `swap()`, `merge()`, `extract()` and node handles.
* `key_comp()` and `value_comp()`.
* Changing a value through an iterator: iterators are read only.
* Reverse iterators.
* Allocators, constructors other than the default and the copy
  constructor, comparison of maps, move operations (a "move" copies),
  and lookup by a type other than `K`.


# What is different


## Putting in

```cpp
template <class... A> std::pair<link, bool> emplace(K const& k, A&&... a);
std::pair<link, bool> insert(K const& k, V const& v);
std::pair<link, bool> insert(K const& k, V&& v);
template <class U = V> std::pair<link, bool> insert_or_assign(K const& k, U&& v);
```

All of them take the key and the value apart, not as a pair, and
return a link (not an iterator) with a `bool` that tells whether a new
key was put in.

`emplace()` constructs the value in place from `a...`, and only if
the key is not in the map - it is `try_emplace()` of the Standard.
`insert()` is `emplace()` with a value to copy or move.

If the key is in the map, `emplace()` and `insert()` change nothing
and return its link and `false`. `insert_or_assign()` assigns `v` to
its value and returns its link and `false`.

If the key is not in the map and there is no room, all of them put
nothing in and return `{ nil, false }`.


## Finding

```cpp
iterator find(K const& k) const;
template <template <class> class O> O<V> find(K const& k) const;
bool      contains(K const& k) const;
size_type count(K const& k) const;
```

`find(k)` is as in the Standard, except that the iterator is always
read only.

`find<O>(k)` returns a copy of the value in a generic optional, or
nothing if the key is not in:

```cpp
if (auto port = ports.find<std::optional>("ssh")) {
    connect(*port);
}
```


## Erasing

```cpp
size_type erase(K const& k);
iterator  erase(const_iterator pos);
link      erase(link pos);
```

By a key and by an iterator, as in the Standard - except that erasing
by an iterator that is at no element, or is not of this very map, is
not an error: nothing is done, and `end()` is returned.

By a link: takes out what is at the link and returns the link of the
next key in order. Returns `nil` if there is none, and also if there
was nothing at the link.


## Iterators

`iterator` and `const_iterator` are the same type, and it refers to a
`keyval` that can only be read. Forward for `cobmap`, bidirectional
for `cobtreemap`. Otherwise as in [common.md](common.md).


# What is added


## Change a value where it is

```cpp
template <class F> bool with(K const& k, F&& f);
template <class F> bool with(K const& k, F&& f) const;
template <class F> bool with(link l, F&& f);
template <class F> bool with(link l, F&& f) const;
```

Calls `f` with a reference to the value of the key `k`, or of the key
at the link `l`, and returns `true`. If there is no such key, `f` is
not called and `false` is returned.

This is what takes the place of `operator[]` and `at()`:

```cpp
if (!counts.with(word, [](int& n) { ++n; })) {
    counts.insert(word, 1);
}
```

`f` gets the value only. The key can't be changed.


## Go over all of it

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
```

Calls `f(key, value)` for each key, in the order of the keys. The key
is a reference to `const`; the value can be changed, unless the map
is `const`.


## The interface by links

```cpp
link where(K const& k) const;
link lower_bound(K const& k) const;
link upper_bound(K const& k) const;
link lbegin() const;
link lfront() const;
link lend() const;
link fwd(link l) const;
bool live(link l) const;
bool contains(link l) const;
bool contains(const_iterator pos) const;
template <template <class> class O> O<V> value(link l) const;
template <template <class> class O> O<K> key(link l) const;
```

`where(k)` is the link of the key `k`, or `nil` if it is not in.

`lower_bound(k)` is the link of the first key that is not ordered
before `k`, and `upper_bound(k)` of the first key that `k` is ordered
before; `nil` if there is none. Note that these two return links,
where `std::map` returns iterators.

`lbegin()` and `lfront()` are the link of the first key, `lend()` is
`nil`, and `fwd(l)` is the link of the key after the one at `l`. See
[cobskiplist.md](cobskiplist.md) for what `fwd()` gives for the link
of no element. There is no `lback()` or `bck()`, also in `cobtreemap`.

`live(l)` and `contains(l)` both tell whether there is a key at the
link; `contains(pos)` whether the iterator is of this very map and at
a key.

`value<O>(l)` and `key<O>(l)` return a copy of the value and of the
key at the link in a generic optional, or nothing if there is no key
at the link.

```cpp
for (auto l = m.lower_bound(from); l != m.nil; l = m.fwd(l)) {
    m.with(l, [](V& v) { /* ... */ });
}
```


## Is it full

```cpp
bool full() const;
```

Whether there is no room for another key.


# What it takes

What the set it is a view of takes ([cobskiplist.md](cobskiplist.md)),
for an element that is a `K` and a `V`.
