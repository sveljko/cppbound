# Ordered sets: the skiplist and the tree

```cpp
#include "cobskiplist.hpp"      // or "atbskiplist.hpp"

cobskiplist<T, N, CMP, RNG>             atbskiplist<T, Tag, CMP, RNG>
cobcheckedskiplist<T, N, CMP, RNG>      atbcheckedskiplist<T, Tag, CMP, RNG>
cobhiveskiplist<T, N, CMP, RNG>         atbhiveskiplist<T, Tag, CMP, RNG>
cobcheckedhiveskiplist<T, N, CMP, RNG>  atbcheckedhiveskiplist<T, Tag, CMP, RNG>

#include "cobtree.hpp"          // or "atbtree.hpp"

cobtree<T, N, CMP>                      atbtree<T, Tag, CMP>
cobcheckedtree<T, N, CMP>               atbcheckedtree<T, Tag, CMP>
cobhivetree<T, N, CMP>                  atbhivetree<T, Tag, CMP>
cobcheckedhivetree<T, N, CMP>           atbcheckedhivetree<T, Tag, CMP>
```

Both are a set of at most `N` unique elements, ordered by `CMP`
(by default `cobhlp::less<T>`, which is `<`), with the interface of
`std::set`. This document says how they differ from `std::set`, and
from each other. For keys with values, see [cobmap.md](cobmap.md).

The elements are kept in a pool ([cobpool.md](cobpool.md)) that is a
part of the set. Read [common.md](common.md) first: what it says
about capacity, links, checked links, the hive option and iterators
all goes for these two.


# Which one

|                              | `cobskiplist`        | `cobtree`              |
|------------------------------|----------------------|------------------------|
| What it is                   | a skiplist           | an AVL tree            |
| Find, insert, erase          | O(log n) on average  | O(log n) in worst case |
| The slowest a single one gets| O(n)                 | O(log n)               |
| Iterators                    | forward              | bidirectional          |
| Links kept for each slot     | log2(N) + 1          | three, and a byte      |
| Needs a random generator     | yes                  | no                     |

Where the time an operation may take has to be bound, use the tree.
It is an AVL tree because, of the balanced trees, that one has the
least height (at most about 1.44 log2(n)), and the height is what
bounds each operation. Nothing in the tree is recursive.

A skiplist is simpler, and as fast on average. How it is laid out
depends on the random numbers it draws; see "The random generator"
below.


# What is not there

Of `std::set`, these are missing:

* Allocators, and the constructors other than the default and the
  copy constructor. There is no construction or assignment from a
  range or an initializer list.
* `contains(key)`. Use `count(key)`, or compare `find(key)` with
  `end()`. (`contains()` is there, but for a link and for an
  iterator; see below.)
* `equal_range()`.
* `insert()` of a range or an initializer list, and `insert()` and
  `emplace_hint()` with a hint.
* `erase()` of a range.
* `swap()`, `merge()`, `extract()` and node handles.
* `key_comp()` and `value_comp()`. (The types `key_compare` and
  `value_compare` are there.)
* Reverse iterators.
* Comparison of sets (`==`, `<=>`).
* Lookup by a type other than `T` ("transparent" comparison).
* Move construction and assignment (a "move" copies).


# What is different


## Inserting can fail for the lack of room

```cpp
std::pair<iterator, bool> insert(T const& v);
std::pair<iterator, bool> insert(T&& v);
template <class... A> std::pair<iterator, bool> emplace(A&&... a);
```

As in the Standard: if an equivalent element is in the set, the
iterator is to that one and the `bool` is `false`.

But if the element is not in and there is no room, nothing is put in
and the result is `{ end(), false }`. So `false` alone does not tell
that the element is in the set: look at the iterator too.

`emplace()` constructs a `T` from its arguments and then inserts it,
so the object is constructed (and destroyed) also when it turns out
not to be put in.


## Erasing by an iterator

```cpp
iterator erase(const_iterator pos);
```

As in the Standard, except that it is not an error if there is nothing
to erase: when `pos` is at no element, or is not an iterator of this
very set, nothing is done and `end()` is returned.


## Iterators

There is only one iterator type: `iterator` is `const_iterator`. As
in `std::set`, an element can't be changed through it, as that could
break the order.

Forward for the skiplist, bidirectional for the tree; for the tree,
`--` of `end()` is the last element. Otherwise they behave as
[common.md](common.md) says.

The elements stay where they are for as long as they are in the set,
so an iterator is good until its very element is erased, as in the
Standard.

Going from an element to the next one is O(1) in the skiplist. In the
tree it is O(log n) in the worst case and O(1) on average, so that
going over all the elements is O(n).

```cpp
bool contains(const_iterator pos) const;
```

Tells whether the iterator is of this very set and at an element.


## Erasing by an iterator, or a link, is not O(1)

In the skiplist, erasing by an iterator or by a link costs what
erasing by value does, O(log n) on average: the skiplist is singly
linked, so the element has to be found again to be unlinked.

In the tree it is O(log n) in the worst case, as everything is.


# What is added


## The interface by links

```cpp
link lbegin() const;
link lfront() const;
link lend() const;
link fwd(link l) const;
link lback() const;         // cobtree
link bck(link l) const;     // cobtree
bool live(link l) const;
bool contains(link l) const;
link erase(link pos);
```

`lbegin()` and `lfront()` are both the link of the smallest element,
and `lback()` of the greatest one; `nil` if the set is empty.
`lend()` is `nil`.

`fwd(l)` is the link of the next element in order, and `bck(l)` of
the previous one; `nil` if there is none. If there is no element at
`l`, a checked set gives `nil`, and one that is not checked gives a
link that is not specified (see [coblist.md](coblist.md), which goes
for these two as well).

`live(l)` and `contains(l)` both tell whether there is an element at
the link, in O(1).

`erase(pos)` takes out the element at `pos` and returns the link of
the next one in order. It returns `nil` if there is none, and also if
nothing was erased because there was no element at `pos`.

There is no `get()`, `set()` or `with()` by a link, as the lists have:
the elements of a set are only read, and that is done by iterators.
(The maps do have them, for the values.)

In the skiplist, `fwd()` takes one more argument:

```cpp
link fwd(link l, level lvl = level{}) const;    // cobskiplist
```

The next element at the given level of the skiplist. Level 0, the
default, has all the elements. This is for looking into how a
skiplist is laid out, not for everyday use.


## Is it full

```cpp
bool full() const;
```

Whether there is no room for another element.


## The random generator: skiplist only

```cpp
cobskiplist<int, 64, cobhlp::less<int>, MyRng> s;
```

A skiplist draws a random number for each element it inserts. `RNG`
is the type of the generator: it has to be default constructible and
callable with no arguments, giving an unsigned integer. Each skiplist
has a generator of its own.

The default, `cobhlp::xorshift32`, is small, fast and always starts
the same, so the same insertions always build the same skiplist. That
is good for testing and for being able to repeat what went wrong. It
is not good if whoever chooses the elements may want to slow you
down: they can work out an order of insertions that makes the
skiplist as slow as a list. Then give it a generator that can't be
predicted - or use the tree.

An `atbskiplist` has as many levels as its dimension calls for.


## Printing: skiplist only

```cpp
template <class STREAM> void printme(STREAM& out) const;
```

Writes the links of the skiplist to `out`: the first element of each
level, then, for each slot, what follows it at each level. For
debugging.


## Checking the tree

```cpp
unsigned height() const;        // cobtree
bool     sound() const;         // cobtree
```

`height()` is the number of elements on the longest way from the root
down: 0 for an empty tree. O(log n).

`sound()` tells whether the tree is as it should be: the elements in
order, the links consistent with one another, and no node with
subtrees that differ in height by more than one. It looks at
everything, so it is O(n) and more. It is for testing; it can only
return `false` if there is a bug in the tree, or if the order that
`CMP` gives has changed under it.


# What it takes

For each of the `N` slots: room for a `T`, a byte (or a bit, in the
hive variants) that tells whether the slot is in use, in the checked
variants an `unsigned`, and

* for the tree, three links and a byte;
* for the skiplist, a link for each level the slot may be at. The
  number of levels is log2(N) + 1, so that is more room than the tree
  takes, though only about two of the links of an element are in use
  on average.

A link is an `int` for a `cob` set and a `std::size_t` for an `atb`
one.
