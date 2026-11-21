# Object pool

```cpp
#include "cobpool.hpp"      // or "atbpool.hpp"

cobpool<T, N>           atbpool<T, Tag>
cobcheckedpool<T, N>    atbcheckedpool<T, Tag>
cobhive<T, N>           atbhive<T, Tag>
cobcheckedhive<T, N>    atbcheckedhive<T, Tag>
```

A fixed number of slots, `N`. Objects of type `T` are made in them and
dropped from them one by one, in any order, and each object is
referred to by a _handle_. An object stays in its slot until it is
dropped, so its handle is good for as long as the object is there.

There is no Standard counterpart of the pool. The nearest thing is
`std::hive` (C++26), which `cobhive` is modelled on; how they differ is
at the end.

The lists, skiplists, trees, maps, the cache and most of the timers
keep their elements in a pool. What is said here about handles is
what [common.md](common.md) says about their links.

`T` can be of any type (see "Elements of any type" in
[common.md](common.md)).

Making and dropping an object, and all access by a handle, are O(1).


# The handle

```cpp
using handle = /* ... */;
static constexpr auto nil;
```

A handle tells where an object is. It is got from `make()`, and from
`first()` and `next()` of a hive, and compares with `nil`, which is
the handle of no object.

For `cobpool` and `cobhive` it is a `cobint<0, N>`. For the checked
ones it is a class of its own, which can be default constructed (to
`nil`), copied, and compared for equality.

The handle of an object that was dropped is harmless: everything
that takes a handle does nothing if there is no object at it. But once
a new object is made in the same slot, the old handle is the handle
of that new object. In `cobcheckedpool` and `cobcheckedhive` it is
not: there, the handle of an object that was dropped stays the handle
of nothing. See "Checked links" in [common.md](common.md) for how,
and what it costs.

Handles are good for a copy of the pool as well: the copy has the same
objects in the same slots.


# Make an object

```cpp
template <class... A> handle make(A&&... a);
```

Constructs a `T` from `a...` in a slot that is free and returns its
handle. If there is no free slot, constructs nothing and returns
`nil`.

Which slot is used is not specified. As it happens, it is the one
that was freed last.


# Drop an object

```cpp
bool drop(handle h);
```

Destroys the object at `h` and frees its slot. Returns `false`, and
does nothing, if there is no object at `h`.


# Is there an object

```cpp
bool live(handle h) const;
```

Whether there is an object at `h`.


# Access an object

```cpp
template <class F> bool with(handle h, F&& f);
template <class F> bool with(handle h, F&& f) const;
T get(handle h) const;
```

`with()` calls `f` with a reference to the object at `h` and returns
`true`. If there is no object at `h`, `f` is not called and `false` is
returned.

`get()` returns a copy of the object, or `T{}` if there is none - so
it can't tell an object that equals `T{}` from no object. Use `live()`
or `with()` when that matters.


# Go over all the objects

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
```

Calls `f` with a reference to each object, in the order of the slots.
That is not the order the objects were made in.

In `cobpool` this looks at every slot, however few objects there are.
In `cobhive` slots that hold no object are skipped 64 at a time.


# Go from object to object: hive only

```cpp
handle first() const;
handle next(handle h) const;

for (auto h = p.first(); h != p.nil; h = p.next(h)) {
    p.with(h, [](T& x) { /* ... */ });
}
```

These two are only in `cobhive` and `cobcheckedhive` (and their `atb`
variants). In the other pools they do not compile.

`first()` is the handle of the object in the first slot that holds
one, and `next(h)` that of the object in the first such slot after
the slot of `h`. Both are `nil` if there is no such slot.

It is fine to drop objects, the one at `h` included, and to make new
ones while going over the pool this way. Only, ask for `next(h)`
before dropping the object at `h` if the pool is checked:

* In `cobhive`, `next(h)` goes by the slot of `h`, whether there is
  an object in it or not.
* In `cobcheckedhive`, `next(h)` is `nil` if the object of `h` is not
  there any more.

An object made while going over the pool is come across if its slot
is after the one the walk is at, and not otherwise.

Both take a number of steps that depends on how many slots without an
object there are in between; a word of 64 slots is one step.


# Size and clearing

```cpp
size_type size() const noexcept;
size_type max_size() const noexcept;
bool      empty() const noexcept;
bool      full() const noexcept;
void      clear();
```

`clear()` drops all the objects. In a checked pool, all the handles
that were given out are then handles of nothing.


# What it takes

For each slot: room for a `T`, a link (an `int` for a `cob` pool, a
`std::size_t` for an `atb` one) and

| Pool             | Whether the slot is in use | Checking      |
|------------------|----------------------------|---------------|
| `cobpool`        | a byte                     | -             |
| `cobcheckedpool` | a byte                     | an `unsigned` |
| `cobhive`        | a bit                      | -             |
| `cobcheckedhive` | a bit                      | an `unsigned` |

The pool with a bit for each slot is a little slower at telling
whether a given slot is in use, which is what every access by a
handle does, and much faster at finding the next slot that is in use.


# How `cobhive` differs from `std::hive`

* The capacity is fixed. There are no blocks, nothing is allocated,
  and so there is no `reserve()`, `shrink_to_fit()`, `trim_capacity()`
  or block capacity limits. `make()` returns `nil` when there is no
  room.
* There are no iterators. Objects are gone over by `for_each()`, or by
  handles with `first()` and `next()`.
* An object is referred to by a handle, not by an iterator or a
  pointer. A handle is good for a copy of the hive too, and with
  `cobcheckedhive` it can be asked whether its object is still there.
* `make()` is `emplace()`, `drop()` is `erase()`; there is no
  `insert()` of a range, no `splice()`, `sort()`, `unique()` or
  `get_iterator()`.
* A slot that was freed is the first to be used again, as in
  `std::hive`, but the order of going over the objects is strictly
  that of the slots.
