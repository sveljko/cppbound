# Binary heap

```cpp
#include "cobbinheap.hpp"   // or "atbbinheap.hpp"

cobbinheap<T, N, CMP>       atbbinheap<T, Tag, CMP>
```

A priority queue of at most `N` elements, kept as a binary heap in a
vector ([cobvector.md](cobvector.md)). It is what
`std::priority_queue` is, and this document says how it differs from
that one.

As in `std::priority_queue`, the element at the top is the one that
no other is ordered after. So with the default `CMP`,
`cobhlp::less<T>`, the greatest element is at the top; to have the
smallest one there, order by `std::greater<T>`:

```cpp
cobbinheap<int, 16, std::greater<int>> h;
h.push(3);
h.push(1);
auto least = h.pop<std::optional>();    // 1
```

Read [common.md](common.md) for what is said there about capacity and
the generic optional `V`. The elements have to be such that they can
be swapped.

Putting in and taking out are O(log n); looking at the top is O(1).


# What is not there

Of `std::priority_queue`, these are missing:

* The choice of the container it is kept in: it is always the vector
  of this library.
* Construction from a range or from a container, and from a
  comparison object: `CMP` is default constructed whenever it is
  needed, so it can't have a state.
* `swap()`.
* Move operations (a "move" copies).


# What is different


## Putting in can fail

```cpp
template <class... A> bool emplace(A&&... a);
bool push(T const& t);
bool push(T&& t);
```

Return `false`, and do nothing, if there is no room.


## Taking out gives the element

```cpp
template <template <class> class V> V<T> pop();
```

Takes the top out and returns it, moved into a generic optional, or
nothing if the heap is empty. In the Standard, `pop()` returns
nothing, and is undefined for an empty queue.


## The top is a copy

```cpp
template <template <class> class V> V<T> top() const;
```

Returns a copy of the top in a generic optional, not a reference;
nothing if the heap is empty.


# What is added

```cpp
template <class F> bool with_top(F&& f) const;
template <class F> void for_each(F&& f) const;
bool full() const noexcept;
void clear();
```

`with_top()` calls `f` with a reference to `const` to the top and
returns `true`; if the heap is empty, `f` is not called and `false`
is returned. It is the way to look at the top of a heap of what
can't be copied.

`for_each()` calls `f` with a reference to `const` to each element.
The order is that of the heap, which is no order to speak of - only
the first one is known to be the top.

`full()` tells whether there is no room for another element, and
`clear()` takes all of them out.


# What it takes

Room for `N` elements, and the size.
