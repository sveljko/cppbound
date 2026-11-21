# Range bound integers

```cpp
#include "cobi.hpp"

cobi<T, D, G>           // an integer of type T, from D to G
cobint<D, G>            // cobi<int, D, G>
cobic<D>                // a constant: an object of type cobint<D, D>
cobirange<T, D, G>      // the range of all cobi<T, D, G>, for range-for
```

A `cobi<T, D, G>` is an integer of type `T` whose value is known, at
compile time, to be at least `D` and at most `G`. It can't hold a
value outside that range, and arithmetic with it can't overflow: the
range of each result is worked out at compile time, and a result
that might not fit its type does not compile.

There is no Standard counterpart. It is what a range type is in Ada.

It is the size of a `T`, and what the compiler generates for it is
what it generates for a `T`: the bounds exist at compile time only.

Everything here can be used in constant expressions.

The [README](README.md) has an introduction, with the reasoning and
remarks on compiler messages. This is the reference.


# The types

```cpp
template <class T, T D, T G = D> struct cobi;
template <int D, int G = D> using cobint = cobi<int, D, G>;
template <int D, int G = D> constexpr auto cobic = cobi<int, D, G>{};
```

`T` is an integer type, `D` the least value and `G` the greatest one
(`D <= G`, or it does not compile). If `G` is left out, it is `D`:
a range of one value, a constant.

`cobint` is the shorthand for when `T` is `int`, which is most of the
time.

`cobic<D>` is not a type but an object: the constant `D`, of the type
`cobint<D, D>`. It is how a number that is known at compile time is
written where a range bound integer is asked for:

```cpp
cobarray<int, 8> a;
a.set(cobic<3>, 1);     // 3 is known to be within 0..7
```

The bounds are also there as members:

```cpp
static constexpr T DD = D;
static constexpr T GG = G;
```


# Construction and assignment

```cpp
constexpr cobi();
template <T PD, T PG> constexpr cobi(cobi<T, PD, PG> const& a);
template <T PD, T PG> constexpr cobi& operator=(cobi<T, PD, PG> a);
```

A default constructed one has the least value, `D`.

There is no constructor from a `T`, on purpose. A plain number gets
in through `be()`, which checks it, or as a `cobic`.

A `cobi` converts to, and can be assigned to, another `cobi` of the
same `T` whose range is the same or wider - that is, whenever the
value is known to fit. A conversion to a narrower range does not
compile (and `std::is_convertible` tells the truth about it):

```cpp
cobint<0, 5>  x;
cobint<0, 10> y = x;    // fine
cobint<0, 3>  z = x;    // does not compile
```

When the value is known to you to fit, but not to the compiler, go
through `be()`.


# Give it a value

```cpp
constexpr bool be(T x);
template <template <class> class V> static constexpr V<cobi> be(T x);
static constexpr bool could(T x);
```

`x.be(n)` gives `x` the value `n` and returns `true`, if `n` is
within the range. Otherwise `x` is as it was, and `false` is
returned. This is the one way a number that is not known at compile
time gets to be a range bound integer.

```cpp
cobint<0, 5> port;
if (!port.be(n)) {
    // n is not a port number
}
```

The static `be<V>(n)` returns a new one, in a generic optional (see
[common.md](common.md)), or nothing if `n` is not within the range:

```cpp
auto port = cobint<0, 5>::be<std::optional>(n);
```

`could(n)` only tells whether `n` is within the range.

```cpp
static constexpr cobi clamp(T x);
```

Returns the value of the range that is nearest to `x`: `x` itself if
it is within the range, `D` if it is less, `G` if it is greater. It
always gives a value, so use it only where the nearest value is
really what is wanted.

```cpp
static constexpr cobi smallest();
static constexpr cobi greatest();
```

The least and the greatest value of the range.


# Get the value

```cpp
constexpr T get() const;
```

The number, as a `T`. From here on the compiler knows nothing about
its range any more.

```cpp
template <class U> constexpr cobi<U, D, G> as() const;
```

The same value and range, as another integer type `U`. It compiles
only if the whole range fits in `U`. This is how integers of
different types are mixed, as the operators want both operands to be
of the same `T`:

```cpp
auto w = cobint<0, 5>{}.as<long long>() + cobi<long long, 1LL << 40>{};
```

```cpp
template <class CH, class TR> std::basic_ostream<CH, TR>& operator<<(std::basic_ostream<CH, TR>& os, cobi x);
```

Writes the number to a stream. Character types are written as
numbers, too.


# Step through the range

```cpp
constexpr bool advance();
constexpr bool ebb();
constexpr void advance_wrap();
constexpr void ebb_wrap();
```

`advance()` makes the value one greater and returns `true`. If it is
the greatest value of the range, it stays as it is and `false` is
returned. `ebb()` does the same the other way.

That makes for a loop over the whole range, which works for any
range, including one that goes up to the greatest value of `T`:

```cpp
cobint<LOWER, UPPER> i;
do {
    // use `i`
} while (i.advance());
```

and backwards:

```cpp
auto i = cobint<LOWER, UPPER>::greatest();
do {
    // use `i`
} while (i.ebb());
```

`advance_wrap()` and `ebb_wrap()` do not stop at the end: from the
greatest value, `advance_wrap()` goes to the least one, and
`ebb_wrap()` the other way around. They are what a position in a
ring is moved with.


# Range-for over the values

```cpp
static constexpr cobrng<T, D, G> range();

for (auto i : cobint<0, 5>::range()) {
    // i is an int: 0, 1, 2, 3, 4, 5
}
```

`range()` is a range of all the values, as plain numbers of type `T`.
It can be given to range-for and to the Standard algorithms.

```cpp
template <class T, T D, T G = D> struct cobirange;

for (auto i : cobirange<int, 0, 5>{}) {
    // i is a cobint<0, 5>
}
```

`cobirange` is the same range, but what it gives are range bound
integers, ready to index with. It is what `irange()` of the arrays
returns.

Both use one value past the greatest one as the end mark, so neither
compiles for a range whose `G` is the greatest value of `T`. For such
a range, use the loop with `advance()`.


# Arithmetic

```cpp
cobi<T, D1 + D2, G1 + G2>   operator+(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
cobi<T, D1 - G2, G1 - D2>   operator-(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
cobi<T, /* least */, /* greatest */>  operator*(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
cobi<T, /* least */, /* greatest */>  operator/(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
cobi<T, /* least */, /* greatest */>  operator%(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
cobi<T, -G, -D>             operator-(cobi<T, D, G> x);
```

The operands can be of any ranges (of the same `T`). The result is a
`cobi` whose range is that of all the results there can be: for `+`,
from the sum of the least values to the sum of the greatest ones, and
so on. You compute as you would with numbers, and the range is kept
track of for you.

```cpp
cobint<0, 2> x;
cobint<2, 3> y;
auto         z = x + y;     // cobint<2, 5>
cobint<0, 5> u = x + y;     // fine: 2..5 is within 0..5
cobint<0, 4> v = x + y;     // does not compile: it might be 5
```

When the range of a result does not fit where you want to put it,
either the computation is wrong, or you know something that the
compiler does not - and then `be()` is where you say so, and deal
with it not being so.

What does not compile:

* A result whose range does not fit in `T`: that is an overflow.
  `auto` does not help, as there is no type for the result to have.
* Division, or `%`, by a range that has 0 in it. It is not enough
  that the divisor is not 0 this time; it has to be known that it
  never is. To divide by something that might be 0, first put it
  into a range that does not have 0, with `be()`.
* Negating, or dividing, a range that has the least value of a
  signed `T` in it (as its opposite does not fit).
* Negating an unsigned integer.

There are no compound assignments (`+=`, `++` and the like): the
result of `x + 1` has a range other than that of `x`, so it can't go
back into `x`. To move through a range, use `advance()` and `ebb()`.

There are no bitwise operators either. The range of their result is
not simple to work out, and these are meant to be numbers, not bits.


# Comparison

```cpp
==  !=  <  <=  >  >=  <=>
```

A `cobi` compares with a `cobi` of any range (of the same `T`), and
with a plain `T`, on either side.

When the ranges of two `cobi` do not overlap, the answer to `==` and
`<` is known from the types alone, and no comparison is made at run
time:

```cpp
cobint<-5, -1> x;
cobint<0, 5>   y;
constexpr bool same = (x == y);     // false, whatever the values
```


# Least, greatest, absolute value

```cpp
constexpr auto max(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
constexpr auto min(cobi<T, D1, G1> x, cobi<T, D2, G2> y);
constexpr auto abs(cobi<T, D, G> x);
```

As the functions of the same names for numbers, with the range of the
result worked out as for the operators:

```cpp
auto m = max(cobint<0, 9>{}, cobint<3, 4>{});   // cobint<3, 9>
auto a = abs(cobint<-5, 3>{});                  // cobint<0, 5>
```

They are found by argument dependent lookup, so they are called
without a namespace.


# With the containers

The `cob` containers use these for indices, so that an index is known
to be within the container:

* `cobint<0, N - 1>` is the index of an array, a vector, a bitset, a
  string and, twice, of a matrix.
* `cobint<0, N>` is the link of a list, a set, a map and the handle
  of a pool, with `N` standing for "none" (`nil`).

A constant index is written `cobic<3>`. One that is computed is made
with `be()`, got from the container (`irange()`, `index_of()`,
`first()`), or stepped with `advance()`.
