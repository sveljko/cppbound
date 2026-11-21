# String

```cpp
#include "cobstring.hpp"    // or "atbstring.hpp"

cobstring<N>                atbstring<Tag>
```

A string of at most `N` characters (`char`), with the interface of
`std::string`, less what is unsafe. `N` is at most 255. This document
says how it differs from `std::string`.

See [common.md](common.md) for the generic optional `V`.

All of `cobstring` can be used in constant expressions.


# What it takes

`N + 1` characters, and nothing else: `sizeof(cobstring<15>)` is 16.

The last of them holds the number of characters that can still be
added. With a capacity of 5, it starts as 5, is 3 with two characters
in, and is 0 when the string is full - when it is also the NUL that
terminates the string. A string that is not full is terminated by a
NUL after its last character.

That is why the capacity can't be more than 255: the number has to
fit in a character. `cobstring<256>` does not compile. An `atbstring`
whose dimension is more than 255 has a capacity of 255.


# What is not there

Of `std::string`, these are missing:

* Iterators: `begin()`, `end()` and the rest, and everything that
  takes or returns one.
* References and pointers to characters: `operator[]`, `at()`,
  `front()` and `back()` that return a reference, `data()` and
  `c_str()`. See "Passing the string on" below.
* `reserve()` and `shrink_to_fit()`: the capacity is what it is.
* `copy()`.
* `operator+`. Use `append()` or `+=`, which tell whether it did fit.
* Reading from a stream (`>>`, `getline()`), `std::to_string()` and
  the `sto*()` conversions.
* The overloads that take a `char const*` with a count, another
  string with a position and a count, an initializer list, or a pair
  of iterators. What takes text takes a `std::string_view`, which a
  literal, a `std::string` and a `cobstring` all convert to.
* `starts_with()` and `ends_with()` of a single character.
* Character types other than `char`, traits and allocators.


# What is different


## Construction

```cpp
constexpr bndstring();
template <std::size_t M> constexpr bndstring(char const (&s)[M]);
template <class B2> constexpr bndstring(bndstring<B2> const& x);
template <template <class> class V> static constexpr V<bndstring> from(std::string_view x);
```

A `cobstring` is made from a string literal, or from a `cobstring`
of the same or a smaller capacity. Both are checked at compile time
to fit:

```cpp
cobstring<15> s = "hello";
cobstring<4>  t = "hello";      // does not compile
cobstring<31> u = s;
cobstring<7>  v = u;            // does not compile
```

A literal is taken up to its first NUL.

What is not known to fit at compile time - a `std::string`, a
`std::string_view`, a longer `cobstring` - can't be constructed from.
It goes through `from<V>()`, which returns the string in a generic
optional, or nothing if it does not fit; or through `assign()`.

```cpp
auto name = cobstring<15>::from<std::optional>(input);
if (!name) { /* too long */ }
```

For an `atbstring` nothing is known to fit at compile time, so it has
only the default constructor, `from<V>()` and `assign()`.


## What changes the string says whether it did

```cpp
constexpr bool assign(std::string_view x);
constexpr bool assign(size_type n, char c);
constexpr bool push_back(char c);
constexpr bool append(std::string_view x);
constexpr bool append(size_type n, char c);
constexpr bool operator+=(std::string_view x);
constexpr bool operator+=(char c);
constexpr bool insert(size_type at, std::string_view x);
constexpr bool insert(size_type at, size_type n, char c);
constexpr bool erase(size_type at = 0, size_type n = npos);
constexpr bool replace(size_type at, size_type n, std::string_view x);
constexpr bool resize(size_type n, char c = '\0');
```

In the Standard these return the string (or nothing) and throw if a
position is out of range. Here they return `bool`:

* `true` if it was done.
* `false` if the result would not fit, or if the position `at` is
  past the end of the string. Then the string is as it was: nothing
  is cut short to make it fit.

So the result should be looked at. `+=` returning `bool` means that
it can't be chained.

As in the Standard, a count `n` that reaches past the end of the
string means "to the end", and `npos` is the count that always does.

The argument may be a part of the string itself:
`s.insert(0, s.view().substr(2))` is fine.


## Reading characters

```cpp
constexpr char get(index i) const;
constexpr bool set(index i, char c);
template <template <class> class V> constexpr V<char> front() const;
template <template <class> class V> constexpr V<char> back() const;
template <template <class> class V> constexpr V<char> pop_back();
```

These take the place of `operator[]`, `at()`, `front()`, `back()` and
`pop_back()`.

`get(i)` returns the character at `i`. The index is a
`cobint<0, N - 1>` (an `atbidx<Tag>` for `atbstring`), so it is known
to be within the capacity; if it is past the last character, `get()`
returns NUL.

`set(i, c)` changes the character at `i`. It returns `false`, and
does nothing, if there is no character at `i`: it does not make the
string longer.

`front<V>()` and `back<V>()` return the first and the last character
in a generic optional, and `pop_back<V>()` takes the last one out and
returns it. All three give nothing if the string is empty.


## Searching

```cpp
template <template <class> class V> constexpr V<index> find(std::string_view x, size_type from = 0) const;
template <template <class> class V> constexpr V<index> find(char c, size_type from = 0) const;
template <template <class> class V> constexpr V<index> rfind(std::string_view x, size_type from = npos) const;
template <template <class> class V> constexpr V<index> rfind(char c, size_type from = npos) const;
template <template <class> class V> constexpr V<index> find_first_of(std::string_view x, size_type from = 0) const;
template <template <class> class V> constexpr V<index> find_last_of(std::string_view x, size_type from = npos) const;
template <template <class> class V> constexpr V<index> find_first_not_of(std::string_view x, size_type from = 0) const;
template <template <class> class V> constexpr V<index> find_last_not_of(std::string_view x, size_type from = npos) const;
```

They search as those of the Standard do, but the result is not a
number that might be `npos`. It is the index of the character where
the match starts, in a generic optional, or nothing if there is no
match.

```cpp
if (auto at = s.find<std::optional>(',')) {
    s.set(*at, ';');
}
```

As the result is the index of a character, it can be used with
`get()` and `set()` with no further check. It also means that there
is one case where the Standard finds something and this does not: an
empty string is "found" by `std::string` at the very end of the
string, where there is no character. Here that is nothing.

`contains()`, `starts_with()`, `ends_with()` and `compare()` are as
in the Standard, for a `std::string_view` (and, for `contains()`, a
character).


## Comparing

`==`, `!=`, `<`, `<=`, `>`, `>=` and `<=>` compare with a `cobstring`
of any capacity, an `atbstring`, a `std::string`, a
`std::string_view` and a string literal, as in the Standard.


## Other members

```cpp
constexpr bndstring substr(size_type at = 0, size_type n = npos) const;
```

Returns a string of the same type. If `at` is past the end, the result
is empty; in the Standard, that throws.

```cpp
constexpr size_type capacity() const noexcept;
constexpr size_type max_size() const noexcept;
constexpr bool      full() const noexcept;
```

`capacity()` and `max_size()` are both `N` (or 255, for an
`atbstring` of a greater dimension). `full()`, which is not in the
Standard, tells whether no more characters can be added.

`size_type` is `unsigned`, and so is `npos`, which is only used for
counts.

`size()`, `length()`, `empty()`, `clear()`, `swap()`, writing to a
stream with `<<` and `std::hash` are as in the Standard.


# Passing the string on

```cpp
constexpr std::string_view view() const noexcept;
constexpr operator std::string_view() const noexcept;
```

`view()`, and the conversion to `std::string_view`, is how the string
is given to anything that takes text.

A view refers to the characters of the string, so it is good only
while the string is there and is not changed. A `std::string_view` is
commonly understood to be a parameter, and not something to keep, so
this is what is offered.

There is no `c_str()`, for the same reason turned around: a pointer to
characters is what people do keep. If you need a NUL terminated
string for an interface that takes a `char const*`, you have to copy
the view (say, into a `std::string`). The characters are in fact
terminated by a NUL in the string, but there is no interface that
promises it.


# `atbstring`

As for the other `atb` variants ([atbdim.md](atbdim.md)), with these
particulars:

* The capacity is the dimension of the tag, but not more than 255.
* There is no constructor from a literal or from another string. Use
  `assign()` or `from<V>()`.
* The characters are on the heap.
* It is not usable in constant expressions.
