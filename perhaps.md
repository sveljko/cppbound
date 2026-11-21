# Optionals

```cpp
#include "perhaps.hpp"
perhaps<T>

#include "contingent.hpp"
contingent<T, S>
contingently<T>             // contingent<T>, for where a template is asked for
nothing<T>                  // what "nothing" is for a T
nothing_is<X>               // "nothing" is the value X
nulifier<T, S>
```

Two types that hold a value or nothing, with no way to get at the
value when there is none.

`perhaps<T>` is what `std::optional<T>` is, less what is unsafe: it
keeps a flag next to the value. The first part of this document says
how it differs from `std::optional`.

`contingent<T>` keeps no flag: one of the values that a `T` could
have is taken to mean "nothing". So it is no bigger than a `T`. It
has no Standard counterpart, and the second part describes it.

Both do as the "generic optional" that the containers return their
results in (see [common.md](common.md)):

```cpp
auto a = list.pop_front<perhaps>();
auto b = list.pop_front<contingently>();    // a list of pointers, say
```


# `perhaps`: how it differs from `std::optional`


## What is not there

* `operator*`, `operator->` and `value()`. There is no way to get a
  reference to the value, so there is no way to get one when there
  is no value. What is done with the value is done in a function
  that is given to `and_then()` or `transform()`, or by `value_or()`.
* The conversion to `bool`. Use `has_value()`.
* `std::nullopt` and `std::in_place`. An empty one is default
  constructed; to make one empty, use `reset()`.
* Comparison other than `==` of two `perhaps` of the same type: no
  `<`, no comparison with a plain value or with `std::nullopt`.
* `swap()`, `std::make_optional`, `std::hash` and deduction guides.
* `and_then()`, `or_else()` and `transform()` are there, but see
  below.


## Construction

```cpp
constexpr perhaps() noexcept;
template <class A, class... Args> constexpr perhaps(A&& a, Args&&... args);
template <class U> static auto unit(U&& x);
```

A default constructed one holds nothing.

Given arguments, it holds a `T` constructed from them, in place: what
would be `std::optional<T>(std::in_place, a, b)` is `perhaps<T>(a, b)`.
The constructor is not `explicit`, so a `T` (or what makes one)
converts to a `perhaps<T>`.

`unit(x)` makes a `perhaps` of the type of `x`, holding `x`:
`perhaps<int>::unit(5)`. (Whichever `perhaps` it is called on, the
type of the result is that of the argument.)

Copy, move, `emplace()`, `reset()`, `has_value()` and `value_or()` are
as in the Standard.


## `and_then()`

```cpp
template <class F> constexpr auto and_then(F&& f);     // for &, const&, && and const&&
```

Calls `f` with the value, if there is one.

In the Standard, `f` has to return an optional, and that is what
`and_then()` returns. Here `f` may return anything, or nothing:

* If `f` returns nothing (`void`), so does `and_then()`. This is the
  way to do something with the value if it is there:

  ```cpp
  p.and_then([](std::string& s) { s += "!"; });
  ```

* Otherwise `and_then()` returns what `f` returned; if there is no
  value, a value initialised object of that type. When `f` returns a
  `perhaps`, that is the chaining that the Standard has, as an empty
  `perhaps` is what there is then. When `f` returns something else,
  "nothing" can't be told from a result that happens to be 0 or
  empty:

  ```cpp
  auto n = p.and_then([](std::string const& s) { return s.size(); });   // 0 if there is no string
  ```

`f` gets a reference to the value: to `const` if the `perhaps` is
`const`, and an rvalue if the `perhaps` is one. The reference is good
during the call only.


## `or_else()` and `transform()`

```cpp
template <class F> constexpr auto or_else(F&& f) const&;
template <class F> constexpr auto or_else(F&& f) &&;
template <class F> constexpr auto transform(F&& f) const;
```

`or_else(f)` returns this `perhaps` if it holds a value, and `f()`
otherwise, as in the Standard; `f` should return a `perhaps<T>`.

`transform(f)` returns a `perhaps` of `f(value)`, or an empty one, as
in the Standard. It is there for `const` access only: `f` gets a
reference to `const`.


# `contingent`

```cpp
template <class T, class S = nothing<T>> class contingent;
```

An optional `T` that takes no more room than what it keeps. `S` is
the _policy_ that says which value means "nothing", and how a `T` is
kept; by default it is `nothing<T>`.

```cpp
contingent<int*>                 p;         // nothing is nullptr
contingent<double>               d(1.5);    // nothing is NaN
contingent<int, nothing_is<-1>>  i(3);      // nothing is -1
contingent<cobint<0, 5>>         c;         // nothing is 6
```

Use it where there are many of them, or where the size matters, and
there is a value that a `T` never has in earnest. Otherwise `perhaps`
is the simpler choice: it works for any type and any value.

What it can't do, compared to `perhaps`: there is no access to the
value where it is - the value is copied out for `f` - so to change
it, a new `contingent` is assigned.


## What "nothing" is

There are policies for:

| `T`                 | "Nothing" is                               |
|---------------------|--------------------------------------------|
| a pointer           | `nullptr`                                  |
| `double`, `float`   | NaN                                        |
| `cobi<T, D, G>`     | `G + 1`; `D - 1` if `G` is the greatest `T`|
| any, `nothing_is<X>`| the value `X`                              |

For any other type, either name the value with `nothing_is<X>` (for
types that can be template arguments), or specialise `nothing<T>` for
your type; see "Your own policy" below. `contingent<int>` as such
does not compile: there is no value of an `int` that is nothing to
everyone.

A `contingent` that is given the value that means "nothing" holds
nothing: `contingent<int, nothing_is<-1>>(-1).has_value()` is
`false`. That is the point, and also the thing to be careful with.

For floating point numbers, the NaN is kept and recognised as bits,
not as a number. So it works also with `-ffast-math`, which lets the
compiler assume that there are no NaNs.

For a `cobi`, what is kept is the plain integer, which is how it can
hold a value that is outside the range.


## Construction

```cpp
constexpr contingent();
constexpr contingent(T x);
```

A default constructed one holds nothing. One made from a `T` holds
it - unless that is the value that means "nothing". The constructor
is not `explicit`.

It is copied and assigned as what it keeps is.


## Is there a value

```cpp
constexpr bool has_value() const;
constexpr void reset();
```

`has_value()` tells whether there is a value. `reset()` makes it hold
nothing.


## Use the value

```cpp
template <class F> constexpr auto and_then(F&& f) const;
template <class F> constexpr auto transform(F&& f) const;
template <class F> constexpr auto or_else(F&& f) const;
template <class U = T> constexpr T value_or(U&& x) const;
```

These are as those of `perhaps`:

`and_then(f)` calls `f` with the value, if there is one, and returns
what `f` returned; a value initialised object of that type if there
is no value; nothing at all if `f` returns `void`. `f` is never
called with the value that means "nothing".

`transform(f)` returns a `perhaps` (not a `contingent`: there may be
no "nothing" for the type of the result) of `f(value)`, or an empty
one.

`or_else(f)` returns this if it holds a value, and `f()` otherwise;
`f` should return a `contingent` of the same type.

`value_or(x)` returns the value, or `x` if there is none.


## Compare

```cpp
constexpr bool operator==(contingent const& x) const;
```

Two are equal if both hold nothing, or both hold a value and the
values are equal. So two that hold nothing are equal also when the
value that means "nothing" does not equal itself, as a NaN does not.


## Your own policy

```cpp
template <> struct nothing<my_id> {
    using stored = my_id;
    static constexpr my_id        none() noexcept { return my_id::invalid(); }
    static constexpr bool         is_none(my_id const& x) noexcept { return !x.valid(); }
    static constexpr my_id        pack(my_id x) { return x; }
    static constexpr my_id const& unpack(my_id const& s) { return s; }
};
```

A policy is a struct with:

* `stored`: the type that is kept. Most often `T` itself.
* `none()`: the `stored` value that means "nothing".
* `is_none(s)`: whether the `stored` value `s` means "nothing".
* `pack(x)`: the `stored` value for the `T` value `x`.
* `unpack(s)`: the `T` for the `stored` value `s`.

When `stored` is `T`, deriving from `cobhlp::stored_as_is<T>` gives
`stored`, `pack()` and `unpack()`, so only `none()` and `is_none()`
are left to write.

Either specialise `nothing<T>`, to make it the default for the type,
or give the policy as the second template argument.


## `contingently`

```cpp
template <class T> using contingently = contingent<T>;
```

`contingent` has two template parameters, so it can't be given where
a template with one is asked for, as the generic optional is.
`contingently` can: `list.pop_front<contingently>()`.


## `nulifier`

```cpp
template <class T, class S = nothing<T>> struct nulifier;

cobtmrleanmill<int, 512, ms, nulifier<int, nothing_is<-1>>> mill;
```

Makes the "nulifier" that the lean timer mill needs (see
[timers.md](timers.md)) out of a policy: called with an ID, it
returns `false` if the ID already is the "nothing" of the policy, and
otherwise makes it so and returns `true`. It is only meant for
policies whose `stored` is `T` itself.
