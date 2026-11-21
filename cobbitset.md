# Bitset

```cpp
#include "cobbitset.hpp"    // or "atbbitset.hpp"

cobbitset<N>                atbbitset<Tag>
```

`N` bits, with the interface of `std::bitset`. This document says how
it differs from that one.

The main difference is in how a bit is named. In `std::bitset` it is
a number, checked at run time by `test()` and `set()` (which throw)
and not at all by `operator[]`. Here it is an index that is known to
be within the bitset - a `cobint<0, N - 1>` for `cobbitset`, an
`atbidx<Tag>` for `atbbitset` - so no access is ever out of the
bitset, and that takes no check and no exception:

```cpp
cobbitset<8> b;
b.set(cobic<3>);        // fine
b.set(cobic<8>);        // does not compile
b.set(3);               // does not compile: 3 is just a number
b.maybe_set(n);         // fine: checked at run time, returns false if out
```

See [cobi.md](cobi.md) and [atbdim.md](atbdim.md) for how such an
index is made, and [common.md](common.md) for the generic optional
`V`.

All of `cobbitset` can be used in constant expressions.


# What is not there

Of `std::bitset`, these are missing:

* Construction from a number or from a string. A bitset starts with
  all the bits clear.
* `operator[]`, and with it the reference to a bit.
* `to_string()`, `to_ulong()` and `to_ullong()`.
* The shifts: `<<`, `>>`, `<<=` and `>>=`.
* Reading from and writing to a stream.
* `std::hash`.


# What is different

```cpp
constexpr bool test(index i) const;
constexpr void set(index i, bool v = true);
constexpr void reset(index i);
constexpr void flip(index i);
constexpr void set();
constexpr void reset();
constexpr void flip();
```

Those for one bit take an index, as said above, and never throw.

All of them return nothing, where those of `std::bitset` return the
bitset, so calls can't be chained.

```cpp
static constexpr size_type size();
```

`size()` is a static member.

`count()`, `any()`, `none()`, `all()`, `&=`, `|=`, `^=`, `~`, `&`,
`|`, `^` and `==` are as in the Standard.


# What is added


## Set a bit by a number

```cpp
template <class U> constexpr bool maybe_set(U i, bool v = true);
```

Sets the bit number `i` to `v`, where `i` is any integer. Returns
`false`, and does nothing, if there is no such bit: if `i` is
negative, or not less than the size.

This is the way in for a number that comes from outside. (The other
way is to make an index of it first, which checks it as well.)


## Go over the bits that are set

```cpp
template <template <class> class V> constexpr V<index> first() const;
template <template <class> class V> constexpr V<index> next(index after) const;
template <class F> constexpr void for_each(F&& f) const;
```

`first<V>()` returns the index of the lowest bit that is set, in a
generic optional; nothing if no bit is set.

`next<V>(after)` returns the index of the lowest bit that is set and
is above `after`; nothing if there is none.

`for_each()` calls `f(index)` for each bit that is set, from the
lowest one up.

```cpp
for (auto i = b.first<std::optional>(); i; i = b.next<std::optional>(*i)) {
    use(*i);
}
```

All three skip the bits that are clear 64 at a time. What they give
is an index, so it can be used with `test()` and the like, or with
any array of the same size, with no further check.


# What it takes

A 64 bit word for each 64 bits, or part thereof. Nothing else.
