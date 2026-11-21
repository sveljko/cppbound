# Flat set and flat map

```cpp
#include "cobflatset.hpp"   // or "atbflatset.hpp"

cobflatset<T, N, CMP>       atbflatset<T, Tag, CMP>

#include "cobflatmap.hpp"   // or "atbflatmap.hpp"

cobflatmap<K, V, N, CMP>    atbflatmap<K, V, Tag, CMP>
```

A set of at most `N` unique elements, and a map of at most `N` keys
with their values, kept in order (by `CMP`, by default
`cobhlp::less`) in a vector, one next to another. They are what
`std::flat_set` and `std::flat_map` of C++23 are, and this document
says how they differ from those two.

Read [common.md](common.md) first: what it says about capacity, the
generic optional and iterators goes for these. There are no links,
and no checked or hive variants.


# When to use them

Compared to `cobtree` and `cobskiplist`
([cobskiplist.md](cobskiplist.md)):

* There is nothing but the elements and their number. No links, no
  flags.
* Finding an element is a binary search over memory that is all in
  one place: O(log n) in the worst case, and quicker than the tree.
* Inserting or erasing moves all the elements after the place: O(n).
  As `n` is at most `N`, that is still a bound, and for a few dozen
  elements it is often less work than keeping a tree balanced.
* Nothing stays where it was. An iterator or an index is good until
  the next insert or erase; after that it is safe to use, but refers
  to another element, or to none.
* The elements have to be such that they can be moved by assignment.

So: for what is small, or is filled once and then mostly looked up.
For what is big and changes a lot, or where a reference to an element
has to stay good, use the tree.


# How they differ from the Standard ones


## What is not there

* The container adaptor side of it: they are not built on a container
  of your choice, and there is no `extract()`, `replace()`, `keys()`
  or `values()`. `cobflatmap` keeps keys and values together, not in
  two containers.
* `sorted_unique` construction and insertion, and construction or
  insertion from a range or an initializer list.
* `equal_range()`.
* `erase()` of a range.
* `swap()`, `key_comp()`, `value_comp()`, reverse iterators and lookup
  by a type other than the key.
* Comparison other than `==` of two sets. (The maps do not compare.)
* For the map: `operator[]`, `at()`, `try_emplace()`, and changing a
  value through an iterator. See "The map" below.
* Move operations (a "move" copies).


## Inserting can fail for the lack of room

```cpp
// cobflatset
std::pair<iterator, bool> insert(T const& v);
std::pair<iterator, bool> insert(T&& v);
template <class... A> std::pair<iterator, bool> emplace(A&&... a);

// cobflatmap
template <class... A> std::pair<iterator, bool> emplace(K const& k, A&&... a);
std::pair<iterator, bool> insert(K const& k, V const& v);
std::pair<iterator, bool> insert(K const& k, V&& v);
template <class U = V> std::pair<iterator, bool> insert_or_assign(K const& k, U&& v);
```

As in the Standard: the iterator is to the element (or the key),
whether it was put in now or was there already, and the `bool` tells
which.

But if it is not in and there is no room, nothing is put in and the
result is `{ end(), false }`.

For the map, the key and the value are given apart, not as a pair.
`emplace()` constructs the value in place from `a...`, and only if
the key is not in the map: it is `try_emplace()` of the Standard.
`insert_or_assign()` assigns to the value of a key that is in, and
returns `false` with its iterator.


## Erasing by an iterator

```cpp
iterator erase(const_iterator pos);
```

Not an error if there is nothing to erase: when `pos` is at no element
or is not an iterator of this very container, nothing is done and
`end()` is returned.


## Iterators

There is one iterator type: `iterator` is `const_iterator`, read
only. Random access; they are the constant iterators of the vector
([cobvector.md](cobvector.md)), with all that goes for those.

For the map, an iterator refers to a struct with the members `key`
and `value`, not to a `std::pair`:

```cpp
for (auto const& e : map) {
    use(e.key, e.value);
}
```


# What is added


## The index of an element

```cpp
template <template <class> class V> V<index> index_of(T const& v) const;    // set
template <template <class> class O> O<index> index_of(K const& k) const;    // map
bool live(index i) const;
live_range irange() const;
```

`index_of()` returns the index of the element (or of the key) in a
generic optional, or nothing if it is not in. O(log n).

An `index` is that of the vector: known at compile time to be within
the capacity, and checked at run time for there being an element at
it (see "Access by an index" in [cobvector.md](cobvector.md)). The
elements are in order, so the index of an element is also the number
of elements that are ordered before it.

An index is good until the next insert or erase.

`live(i)` tells whether there is an element at the index, and
`irange()` is the range of the indices there are, in order.


## Access by an index: the set

```cpp
template <template <class> class V> V<T> at(index i) const;
template <template <class> class V> V<T> front() const;
template <template <class> class V> V<T> back() const;
T get(index i) const;
```

`at<V>(i)` returns a copy of the element at the index in a generic
optional, or nothing if there is none. `front<V>()` is the smallest
element and `back<V>()` the greatest one. `get(i)` returns a copy, or
`T{}` if there is no element at the index.


## Go over the elements

```cpp
template <class F> void for_each(F&& f) const;      // set

template <class F> void for_each(F&& f);            // map
template <class F> void for_each(F&& f) const;      // map
```

For the set, calls `f` with a reference to `const` to each element, in
order. For the map, calls `f(key, value)` for each key, in the order
of the keys; the value can be changed, unless the map is `const`.


## Capacity

```cpp
size_type capacity() const noexcept;
bool      full() const noexcept;
```

`capacity()` is `max_size()`: `N`.


# The map

As in the other maps of this library ([cobmap.md](cobmap.md)), no
reference to a value is given out, so there is no `operator[]` and no
`at()`. What there is instead:

```cpp
iterator find(K const& k) const;
template <template <class> class O> O<V> find(K const& k) const;
template <template <class> class O> O<K> key(index i) const;
template <template <class> class O> O<V> value(index i) const;
template <class F> bool with(K const& k, F&& f);
template <class F> bool with(K const& k, F&& f) const;
template <class F> bool with(index i, F&& f);
```

`find(k)` is as in the Standard, with a read only iterator.
`find<O>(k)` returns a copy of the value of the key in a generic
optional, or nothing if the key is not in.

`key<O>(i)` and `value<O>(i)` return a copy of the key and of the
value at the index, or nothing if there is none.

`with()` calls `f` with a reference to the value of the key `k`, or of
the key at the index `i`, and returns `true`. If there is no such key,
`f` is not called and `false` is returned. This is how a value is
changed:

```cpp
cobflatmap<std::string, int, 16> ports;
ports.insert("http", 80);
ports.with("http", [](int& p) { p = 8080; });
auto p = ports.find<std::optional>("http");     // 8080
```

`lower_bound()` and `upper_bound()` return iterators, as in the
Standard (in `cobmap` they return links).

The type of what is kept, `cobhlp::flatkeyval<K, V>`, has a `key`
member that is not `const`, because what is kept has to be movable.
The key is protected by never being given out as anything but
`const`.


# What it takes

Room for `N` elements - for the map, `N` keys and `N` values - and
the size. Nothing else.
