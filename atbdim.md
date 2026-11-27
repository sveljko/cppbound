# Allocation time bound containers: `atb`

```cpp
#include "atbdim.hpp"

atbdim<Tag>
atbidx<Tag>
atbpart<Tag, NUM, DEN, ADD, MIN>
```

The `cob` containers have a capacity that is known at compile time.
Often it is not: it is read from a configuration, or worked out from
what the machine has, when the program starts - and from then on it
does not change. The `atb` containers are for that. Their capacity is
set at run time, once, and their storage is allocated once, when they
are made. `atb` stands for "allocation time bound".

They are the same code as the `cob` containers, with the same
interface and the same safety: an index is still known, by its type,
to be within the container. There is no Standard counterpart of any
of this.

This document describes how a capacity is set and how indices are
made, and then lists what is different about each `atb` container.


# The tag

```cpp
struct sessions {};     // a tag
```

A capacity that is set at run time is called a _dimension_ here. It
belongs to a type, the _tag_. A tag is any type you choose; an empty
struct made for the purpose is the usual thing. All it does is tell
dimensions apart: every container that names the tag has the
dimension of that tag.

```cpp
atbdim<sessions>::set(config.max_sessions);
atblist<session, sessions>       list;      // room for that many
atbarray<stats, sessions>        stats;     // and as many of these
```

That containers of the same tag are of the same size is what makes an
index of one good for the other.


# Set the dimension

```cpp
static bool atbdim<Tag>::set(std::size_t x) noexcept;
```

Sets the dimension of the tag to `x`. This can be done once, and has
to be done before the dimension is used.

Returns `true` if the dimension was set. Returns `false`, and changes
nothing, if it was set before, or if it was already used - by `get()`
or by anything that needs to know it: making a container, an index or
an iterator of the tag.


# Get the dimension

```cpp
static std::size_t atbdim<Tag>::get() noexcept;
```

Returns the dimension of the tag.

If the dimension was not set, it is 0, and this call makes it so for
good: it can't be set any more. So the dimension changes at most
once, from "not set" to what it then stays, and whoever looks at it,
and whenever, sees the same.

That holds between threads as well. It is fine to use containers of
the same tag in many threads; only, the thread that sets the
dimension has to do it before any other uses it, or it will find
that it is too late (`set()` returns `false`).

With a dimension of 0, the containers of the tag have no room at all,
and everything fails as it does for a container that is full.


# An index of a tag

```cpp
template <class Tag> class atbidx;
```

An `atbidx<Tag>` is a number that is known to be less than the
dimension of `Tag`. So it is a valid index for every array, vector,
bitset, string or matrix of that tag, for as long as the program
runs, and access by it takes no check.

That is so because of how one is made: there is no default
constructor, and no constructor from a number. An index can only be
got from the functions below, all of which check, and it can't be
made before the dimension is final.

This is what `cobint<0, N - 1>` is for the `cob` containers.


## Make an index

```cpp
template <template <class> class V> static V<atbidx> be(std::size_t x);
template <template <class> class V> static V<atbidx> first();
template <template <class> class V> static V<atbidx> last();
```

`be<V>(x)` returns the index `x` in a generic optional (see
[common.md](common.md)), or nothing if `x` is not less than the
dimension.

`first<V>()` is the index 0 and `last<V>()` the greatest index there
is; both are nothing if the dimension is 0.

```cpp
if (auto i = atbidx<sessions>::be<std::optional>(n)) {
    stats.set(*i, {});
}
```


## Go over all the indices

```cpp
static walk range();

for (auto i : atbidx<sessions>::range()) {
    stats.set(i, {});
}
```

The range of all the indices of the tag, from 0 up. This is the way
to go over a whole container with no checks.


## Move an index

```cpp
bool advance();
bool ebb();
```

`advance()` makes the index one greater and returns `true`; if it is
the last index, it stays as it is and `false` is returned. `ebb()`
does the same the other way: one less, or `false` if it is 0.

```cpp
auto i = *atbidx<sessions>::first<std::optional>();     // if there is one
do {
    use(i);
} while (i.advance());
```


## Value and comparison

```cpp
constexpr std::size_t get() const;
constexpr auto operator<=>(atbidx const&) const = default;
```

`get()` is the number. Indices of the same tag compare as their
numbers do, with all six comparisons. Indices of different tags are
different types and do not compare.


# A dimension worked out from another

```cpp
template <class Tag, std::size_t NUM = 1, std::size_t DEN = 1,
          std::size_t ADD = 0, std::size_t MIN = 0>
struct atbpart {};
```

`atbpart<Tag, NUM, DEN, ADD, MIN>` is a tag. Its dimension is not
set, but worked out from the dimension `n` of `Tag`:

* 0, if `n` is less than `MIN`;
* `n / DEN * NUM + ADD` otherwise (the division is done first, and
  rounds down).

`atbdim<atbpart<...>>::set()` always returns `false`.

It is for what is built of several containers that share a
dimension out: the timer wheel of `n` timers gives each of its
`SPOKES` lists room for `atbpart<Tag, 2, SPOKES>` of them. Use it when
you build such a thing yourself:

```cpp
struct points {};
using edges = atbpart<points, 3>;       // three times as many
atbarray<point, points> p;
atbarray<edge, edges>   e;
```

Looking at the dimension of an `atbpart` looks at that of `Tag`, and
so makes it final too.


# The `atb` containers

Each container has an `atb` variant, in a header of the same name
with `atb` for `cob`. Where the `cob` one takes a number, the `atb`
one takes a tag:

| `cob`                          | `atb`                                   |
|--------------------------------|-----------------------------------------|
| `cobarray<T, N>`               | `atbarray<T, Tag>`                      |
| `cobmatrix<T, M, N>`           | `atbmatrix<T, RowTag, ColumnTag>`       |
| `cobvector<T, N>`              | `atbvector<T, Tag>`                     |
| `cobstring<N>`                 | `atbstring<Tag>`                        |
| `cobbitset<N>`                 | `atbbitset<Tag>`                        |
| `cobpool<T, N>`                | `atbpool<T, Tag>`                       |
| `coblist<T, N>`                | `atblist<T, Tag>`                       |
| `cobfwdlist<T, N>`             | `atbfwdlist<T, Tag>`                    |
| `cobskiplist<T, N, CMP, RNG>`  | `atbskiplist<T, Tag, CMP, RNG>`         |
| `cobtree<T, N, CMP>`           | `atbtree<T, Tag, CMP>`                  |
| `cobmap<K, V, N, CMP, RNG>`    | `atbmap<K, V, Tag, CMP, RNG>`           |
| `cobtreemap<K, V, N, CMP>`     | `atbtreemap<K, V, Tag, CMP>`            |
| `cobflatset<T, N, CMP>`        | `atbflatset<T, Tag, CMP>`               |
| `cobflatmap<K, V, N, CMP>`     | `atbflatmap<K, V, Tag, CMP>`            |
| `cobhash<K, V, N, H, E>`       | `atbhash<K, V, Tag, H, E>`              |
| `coblru<K, V, N, H, E>`        | `atblru<K, V, Tag, H, E>`               |
| `cobbinheap<T, N, CMP>`        | `atbbinheap<T, Tag, CMP>`               |
| `cobring<T, N>`                | `atbring<T, Tag>`                       |
| `cobspsc<T, N>`                | `atbspsc<T, Tag>`                       |
| `cobfunction<S, BYTES>`        | `atbfunction<S, Tag>`                   |
| `cobmovefunction<S, BYTES>`    | `atbmovefunction<S, Tag>`               |
| `cobtmrlist<ID, N, U>`         | `atbtmrlist<ID, Tag, U>`                |
| `cobtmrwheel<ID, N, U, SPOKES>`| `atbtmrwheel<ID, Tag, U, SPOKES>`       |
| `cobtmrmill<ID, N, U, ...>`    | `atbtmrmill<ID, Tag, U, ...>`           |
| `cobtmrleanmill<ID, N, U, ...>`| `atbtmrleanmill<ID, Tag, U, ...>`       |
| `cobtmrleanring<ID, N, U, NULIFY>`| `atbtmrleanring<ID, Tag, U, NULIFY>` |

The `checked` and `hive` names go the same way: `atbcheckedlist`,
`atbhivetree`, `atbcheckedhivemap` and so on.

The interface is that of the `cob` container, as its document
describes it. What differs, for all of them:

* **Indices.** Where the `cob` container takes a `cobint<0, N - 1>`,
  the `atb` one takes an `atbidx<Tag>`. That goes for the elements of
  an array, a vector and a matrix, the bits of a bitset and the
  characters of a string.
* **Links and handles** (of pools, lists, sets and maps) are classes
  of their own rather than range bound integers. As before, they are
  got from the container and compared with `nil`.
* **Storage** is on the heap. All of it is allocated when the
  container is made, and none after that. An allocation that fails
  throws `std::bad_alloc`, as any `new` does; that can only happen
  when a container is made or copied.
* **Copies** are deep, and there are no move operations: a "move"
  copies, and what was "moved from" stays as it was.
* **The dimension is looked at** when a container is made, and when
  an iterator of one is default constructed, so it can't be set after
  that. (A default constructed iterator of an `atbarray` is the one
  exception: it does not look.)
* **Nothing is `constexpr`**, and nothing about the size is checked
  at compile time. What a `cob` container refuses to compile - a
  string literal that is too long, a callable that does not fit - an
  `atb` one has to be asked to take, with a function that returns
  whether it did.

And for some of them in particular:

* `atbstring`: the capacity is at most 255, whatever the dimension.
  No constructor from a literal or another string; use `assign()` or
  `from<V>()`. See [cobstring.md](cobstring.md).
* `atbfunction`, `atbmovefunction`: the dimension is the room for
  what is held, in bytes. No constructor from a callable; use
  `assign()`. See [cobfunction.md](cobfunction.md).
* `atbskiplist`, `atbmap`: the skiplist has as many levels as the
  dimension calls for.
* `atbmatrix`: two tags, one for the rows and one for the columns.
  They may be the same.
* `atbarray`: no constructor from a list of values.
* The timers: the dimension is the number of timers, and each spoke
  gets its share of it. See [timers.md](timers.md).


# What makes it sound

Each container allocates one slot more than the dimension, and what
indexes it is at most the dimension. An index can only be made once
the dimension is final, and is less than it. So no index is ever out
of any container of its tag - not because it is checked when used,
but because one that would be can't be made.
