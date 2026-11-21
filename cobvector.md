# Vector

```cpp
#include "cobvector.hpp"    // or "atbvector.hpp"

cobvector<T, N>             atbvector<T, Tag>
```

An array that grows, up to its capacity `N`, and shrinks, with the
interface of `std::vector` (or of `std::inplace_vector` of C++26,
which also has a fixed capacity). This document says how it differs
from `std::vector`.

Read [common.md](common.md) first: what it says about capacity, the
generic optional `V` and iterators goes for the vector. There are no
links, and no checked or hive variants, as the vector does not keep
its elements in a pool.

The elements are next to one another, in the vector object itself
(on the heap, for `atbvector`). Only as many are constructed as there
are in the vector.


# What is not there

Of `std::vector`, these are missing:

* `operator[]`, `at()`, `front()` and `back()` that return a
  reference, and `data()`. See "Access by an index" below.
* `reserve()` and `shrink_to_fit()`: the capacity is what it is.
* Constructors other than the default and the copy constructor. There
  is no construction or assignment from a count, a range or an
  initializer list, and no `assign()`.
* `insert()` of a count, a range or an initializer list, and
  `emplace()` by an iterator.
* Reverse iterators.
* Comparison other than `==`.
* Allocators, the non-member `swap()`, `erase()` and `erase_if()`,
  and move operations (a "move" copies).


# What is different


## Putting in can fail

```cpp
template <class... A> bool emplace_back(A&&... a);
bool push_back(T const& t);
bool push_back(T&& t);
bool resize(size_type n, T const& v = T{});
```

`emplace_back()` and `push_back()` return `false`, and do nothing, if
the vector is full.

`resize()` returns `false`, and does nothing, if `n` is more than the
capacity. As it has the default argument `T{}`, `T` needs a default
constructor for `resize()` to be used at all.


## Taking out gives the element

```cpp
template <template <class> class V> V<T> pop_back();
```

Returns the last element, moved into a generic optional, or nothing if
the vector is empty.


## Inserting and erasing by an iterator

```cpp
iterator insert(const_iterator pos, T const& t);
iterator insert(const_iterator pos, T&& t);
iterator erase(const_iterator pos);
iterator erase(const_iterator first, const_iterator last);
```

As in the Standard, except that misuse is not an error:

* `insert()` returns `end()`, and puts nothing in, if there is no room
  or if `pos` is not an iterator of this very vector.
* `erase(pos)` returns `end()`, and erases nothing, if `pos` is at no
  element or is not of this vector.
* `erase(first, last)` returns `end()`, and erases nothing, if that is
  not a range of this vector.

The elements after the position are moved by assignment, as in the
Standard, so `T` has to be move assignable for these to be used.


## Iterators

Random access. As [common.md](common.md) says, they can't be moved
out of the vector and can always be dereferenced. In detail:

* An iterator keeps the number of its element. So, as in the
  Standard, after an insert or an erase before it, it refers to
  another element - but that is all that happens to it.
* If the vector has shrunk so that there is no element with that
  number any more, the iterator refers to the safety spot. Moving it
  then takes it to `end()`.
* Moving an iterator past the last element, or before the first one,
  takes it to `end()`.
* Iterators of different vectors compare unequal and are not less
  than one another, and their difference is 0.

```cpp
std::sort(v.begin(), v.end());
for (auto& x : v) { /* ... */ }
```


## Swapping

```cpp
void swap(bndvector& x);
bool swap(index i, index j);
```

`swap(x)` exchanges the elements with another vector of the same
type, by copying them: O(n), and `T` has to be copyable.

`swap(i, j)` exchanges two elements of this vector. Returns `false`,
and does nothing, if there is no element at one of the indices.


# Access by an index

```cpp
using index    = /* cobint<0, N - 1>, or atbidx<Tag> */;
using position = /* cobint<0, N>, or the like */;
```

An `index` is known, at compile time, to be within the capacity: it
can't be made from a number that is not. So no access by an index is
ever out of the storage, and that takes no check at run time.

Whether there is an element at an index is another matter. That
depends on the size, which changes at run time, so each access by an
index checks it (one comparison), and has a way to say "there is no
such element". An index kept from before the vector shrank is
harmless.

A `position` is an index, or one more than the last index. It is used
for inserting, where "after the last element" is a place too.

For `cobvector`, an index is made from a number that is known at
compile time with `cobic<3>`, and from one that is not with `be()`,
which checks it (see [cobi.md](cobi.md)). For `atbvector`, see
[atbdim.md](atbdim.md).


## Read an element

```cpp
template <template <class> class V> V<T> at(index i) const;
template <template <class> class V> V<T> front() const;
template <template <class> class V> V<T> back() const;
T get(index i) const;
```

`at<V>(i)` returns a copy of the element at `i` in a generic optional,
or nothing if there is no element there. `front<V>()` and `back<V>()`
do so for the first and the last element.

`get(i)` returns a copy, or `T{}` if there is no element at `i`.


## Change an element

```cpp
bool set(index i, T const& t);
bool set(index i, T&& t);
template <class F> bool with(index i, F&& f);
template <class F> bool with(index i, F&& f) const;
```

`set()` assigns to the element at `i`. `with()` calls `f` with a
reference to it. Both return `false`, and do nothing, if there is no
element at `i`.


## Is there an element

```cpp
bool live(index i) const;
```

Whether there is an element at `i`, that is, whether `i` is less than
the size.


## Insert and erase by an index

```cpp
template <class... A> bool emplace(position pos, A&&... a);
bool insert(position pos, T const& t);
bool insert(position pos, T&& t);
bool erase(index pos);
```

`emplace()` and `insert()` put a new element before the one at `pos`;
`pos` may be the size, which puts it at the end. They return `false`,
and do nothing, if there is no room or if `pos` is more than the
size.

`erase()` takes out the element at `pos`. Returns `false` if there is
none.

The arguments of `emplace()` may refer to an element of the vector.


## Go over the elements

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
live_range irange() const;

for (auto i : v.irange()) {
    use(v.get(i));
}
```

`for_each()` calls `f` with a reference to each element, first to
last.

`irange()` is the range of the indices of the elements there are at
the time it is called, first to last. An access by such an index
can't fail - as long as the vector does not shrink in the loop.


## Capacity

```cpp
size_type capacity() const noexcept;
bool      full() const noexcept;
```

`capacity()` is `max_size()`: `N`. `full()` tells whether the size
has reached it.


# What it takes

Room for `N` objects of type `T`, and the size. Nothing else.
