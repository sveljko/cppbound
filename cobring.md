# Ring buffer

```cpp
#include "cobring.hpp"      // or "atbring.hpp"

cobring<T, N>               atbring<T, Tag>
```

A queue of at most `N` elements: what is put in first is taken out
first. The elements are kept in an array that is used "in a circle",
so nothing is ever moved, and everything here is O(1), except for
what goes over all the elements.

There is no Standard counterpart. It is used like a `std::queue`
with a bound, and can also be used as a buffer that keeps the latest
`N` of what was put in (see `emplace_over()`).

`T` can be of any type; see [common.md](common.md) for that, and for
the generic optional `V`. There are no iterators and no links.

This ring is for one thread. For a queue between two threads, see
[cobspsc.md](cobspsc.md).


# Put in at the back

```cpp
template <class... A> bool emplace_back(A&&... a);
bool push_back(T const& t);
bool push_back(T&& t);
```

Puts a new element in, as the newest one: `emplace_back()` constructs
it from `a...`, `push_back()` copies or moves `t`.

Returns `false`, and does nothing, if the ring is full.


# Put in, dropping the oldest if need be

```cpp
template <class... A> void emplace_over(A&&... a);
```

Like `emplace_back()`, but if the ring is full, the oldest element is
dropped (destroyed) first. So this always puts the element in, and
the ring holds the latest `N` of them.

```cpp
cobring<sample, 100> latest;
latest.emplace_over(read_sensor());     // keeps the last 100 samples
```

The arguments must not refer to the oldest element of a full ring: it
is gone by the time the new one is constructed.


# Take out from the front

```cpp
template <template <class> class V> V<T> pop_front();
```

Takes the oldest element out and returns it, moved into a generic
optional. Returns nothing if the ring is empty.


# Look at an element

```cpp
template <template <class> class V> V<T> front() const;
template <template <class> class V> V<T> back() const;
template <template <class> class V> V<T> at(size_type i) const;
```

Each returns a copy in a generic optional: `front<V>()` of the oldest
element, `back<V>()` of the newest one, and `at<V>(i)` of the `i`-th
oldest, counting from 0 (so `at<V>(0)` is the oldest).

They return nothing if there is no such element: if the ring is
empty or, for `at()`, if `i` is not less than the size.

`i` is a plain number here, not an index that is checked at compile
time: which elements there are changes with every push and pop.


# Change the oldest element where it is

```cpp
template <class F> bool with_front(F&& f);
```

Calls `f` with a reference to the oldest element and returns `true`.
If the ring is empty, `f` is not called and `false` is returned.

Together with `pop_front()` this lets you take an element out only
after it was dealt with:

```cpp
ring.with_front([&](message& m) { done = send(m); });
if (done) {
    ring.pop_front<ignore>();       // see common.md for `ignore`
}
```


# Go over all the elements

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
```

Calls `f` with a reference to each element, oldest to newest.


# Size and clearing

```cpp
size_type size() const noexcept;
size_type max_size() const noexcept;
bool      empty() const noexcept;
bool      full() const noexcept;
void      clear();
```

`clear()` takes all the elements out, destroying them.


# Copying

A ring can be copied, and the copy has copies of the elements, in the
same order. There are no move operations: a "move" copies.


# What it takes

Room for `N` objects of type `T`, the place of the oldest element and
the size. All `N` slots can be in use.
