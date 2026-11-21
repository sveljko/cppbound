# Array

```cpp
#include "cobarray.hpp"     // or "atbarray.hpp"

cobarray<T, N>              atbarray<T, Tag>
```

An array of `N` elements, all of which are there for as long as the
array is. It is what `std::array` is, and this document says how it
differs from that one.

The main difference is in how an element is named. In `std::array`
it is a number, checked at run time by `at()` (which throws) and not
at all by `operator[]`. Here it is an index that is known, at compile
time, to be within the array: a `cobint<0, N - 1>` for `cobarray`, an
`atbidx<Tag>` for `atbarray`. So no access is ever out of the array,
and that takes no check and no exception:

```cpp
cobarray<int, 4> a;
a.set(cobic<3>, 7);         // fine
a.set(cobic<4>, 7);         // does not compile
a.set(3, 7);                // does not compile: 3 is just a number
a.maybe_set(n, 7);          // fine: checked at run time, false if out
```

See [cobi.md](cobi.md) and [atbdim.md](atbdim.md) for how an index is
made, and [common.md](common.md) for iterators.

All of `cobarray` can be used in constant expressions, except for the
iterators.


# What is not there

Of `std::array`, these are missing:

* `operator[]`, `at()` and `data()`, and `front()` and `back()` that
  return a reference. No reference to an element is given out, other
  than through iterators.
* Aggregate initialisation with fewer values than there are
  elements. See "Construction".
* Comparison other than `==`.
* Reverse iterators.
* The tuple interface (`std::get`, `std::tuple_size`),
  `std::to_array` and the non-member `swap()`.
* Arrays of no elements: `N` has to be at least 1.


# What is different


## Construction

```cpp
constexpr bndarray();
template <class... U> constexpr bndarray(U&&... u);     // cobarray: exactly N values

cobarray<int, 3> a;                 // 0, 0, 0
cobarray<int, 3> b{ 1, 2, 3 };
cobarray<int, 3> c{ 1, 2 };         // does not compile
```

A default constructed array has its elements value initialised:
numbers are 0, and objects are default constructed. `T` has to have a
default constructor.

A `cobarray` can be given a value for each element - exactly as many
values as there are elements, which is checked at compile time. An
`atbarray` can't, as its size is not known then.

Copying an array copies the elements. An `atbarray` has no move
operations (a "move" copies).


## The first and the last element are copies

```cpp
constexpr T front() const;
constexpr T back() const;
```

Return a copy, not a reference.


## Iterators

Random access. They behave as [common.md](common.md) says: an
iterator can't be moved out of the array - one that would get out
stays at `end()` - and `end()` can be dereferenced, giving the
"safety spot".

For the array, the safety spot is a part of the array itself: there
is one element more than `N`, which is what `end()` refers to. So
`sizeof(cobarray<T, N>)` is that of `N + 1` objects of type `T`.

That safety costs a comparison each time an iterator is moved. To go
over the array with no check at all, use `irange()`, below.


# What is added


## Access by an index

```cpp
constexpr T get(index i) const;
constexpr T set(index i, T const& t);
constexpr T set(index i, T&& t);
```

`get()` returns a copy of the element at `i`.

`set()` assigns `t` to the element at `i`, and returns a copy of the
element as it then is.

Returning copies has a price for elements that are big. What it buys
is that there is no reference that could outlive the array.


## Access to a member of an element

```cpp
template <class V> constexpr V get(index i, V T::*m) const;
template <class V> constexpr V set(index i, V T::*m, V const& v);
constexpr taken  grab(index i);
constexpr ctaken grab(index i) const;
```

For when `T` is a struct or a class, and copying all of it to get at
one member would be a waste.

`get(i, m)` returns a copy of the member `m` of the element at `i`,
and `set(i, m, v)` assigns `v` to that member and returns a copy of
it as it then is. `m` is a pointer to a data member.

`grab(i)` returns an object whose only use is `->`, to get at a member
of the element at `i`. It can't be copied, moved or kept, so it can't
outlive the expression it is in.

```cpp
struct S {
    int   i;
    float f = 0.0;
};
cobarray<S, 3> as;
as.set(cobic<0>, &S::f, 2.0f);
float x = as.get(cobic<0>, &S::f);      // 2.0
as.grab(cobic<0>)->f = 4.0;
float y = as.grab(cobic<0>)->f;         // 4.0
```


## Set an element by a number

```cpp
template <class U> constexpr bool maybe_set(U i, T const& t);
```

Assigns `t` to the element number `i`, where `i` is any integer.
Returns `false`, and does nothing, if there is no such element: if
`i` is negative, or not less than the size.


## Go over the indices

```cpp
static constexpr auto irange();

for (auto i : a.irange()) {
    a.set(i, a.get(i) + 1);
}
```

The range of all the indices of the array, first to last. What it
gives are indices, so the access in the loop takes no check. This is
both the safe and the fast way to go over an array.


## Make an array of given values

```cpp
static constexpr bndarray filled(T const& v);
template <class F> static constexpr bndarray generate(F f);
constexpr void fill(T const& v);
```

`filled(v)` returns an array with every element a copy of `v`.

`generate(f)` returns an array where the element at each index `i` is
`f(i)`. `f` is called with the index, not with a number, in the order
of the indices.

```cpp
auto squares = cobarray<int, 10>::generate([](cobint<0, 9> i) { return i.get() * i.get(); });
```

`fill(v)` is as in the Standard.


## Swap

```cpp
constexpr void swap(bndarray& x);
```

A member, as in the Standard: swaps the elements one by one.


# `atbarray`

As for the other `atb` variants ([atbdim.md](atbdim.md)). The
elements are on the heap, there is no constructor from values, and it
is not usable in constant expressions.

Unlike with the other `atb` containers, a default constructed
iterator of an `atbarray` does not look at the dimension of the tag,
so it does not stop the dimension from being set afterwards.
