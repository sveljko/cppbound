# Callables

```cpp
#include "cobfunction.hpp"      // or "atbfunction.hpp"

cobfunction<R(A...), BYTES>         atbfunction<R(A...), Tag>
cobmovefunction<R(A...), BYTES>     atbmovefunction<R(A...), Tag>
```

Holds anything that can be called with arguments `A...` to give an
`R` - a function, a lambda, an object with `operator()`, a pointer to
a member - and calls it. `cobfunction` is what `std::function` is,
and `cobmovefunction` what `std::move_only_function` (C++23) is. This
document says how they differ from those two.

There are two main differences:

* What is held is kept in the object itself, in `BYTES` bytes. So
  nothing is ever allocated, and what does not fit is not taken.
* Calling one that holds nothing is not an error.

`BYTES` is by default the size of three pointers: enough for a
function, for a member function together with the object to call it
on, or for a lambda that captures a few pointers or references.


# What is not there

Of `std::function` and `std::move_only_function`, these are missing:

* Construction from, assignment of and comparison with `nullptr`. An
  empty one is default constructed; to make one empty, use `clear()`.
* `target()` and `target_type()`.
* `swap()`.
* Allocators and deduction guides.
* For `cobmovefunction`: the `const`, `noexcept` and reference
  qualifiers in the signature, and `std::in_place_type` construction.


# What is different


## What does not fit does not compile

```cpp
template <class F> bndfunction(F&& f);

cobfunction<void(int)> on_expiry = [&log](int id) { log.note(id); };
cobfunction<int(int), 8> f = [big_array](int) { return 0; };   // does not compile
```

A `cobfunction` can be constructed from, and assigned, anything that

* can be called with `A...`, giving something that converts to `R`,
* can be copied (for `cobmovefunction`: moved), and
* is no bigger than `BYTES`, and not aligned more strictly than
  `std::max_align_t`.

If the first two are not so, there is no such constructor, as in the
Standard. If it is the size that is wrong, that is an error at compile
time, with a message that says so. Either way, nothing ever falls
back to the heap.

To find out how much room something needs, ask `sizeof` of it; for a
lambda, that is about the sum of what it captures.

An `atbfunction` has no such constructor, as nothing is known to fit
at compile time. It is default constructed and then `assign()`ed.


## Calling one that holds nothing

```cpp
R operator()(A... a) const;
```

If it holds something, that is called, as in the Standard.

If it holds nothing, nothing is called, and nothing is thrown. If `R`
is not `void`, a value initialised `R` is returned: 0, `false`, a
null pointer, an empty string. (`std::function` throws
`std::bad_function_call`; for `std::move_only_function` it is
undefined behaviour.)

When you need to know whether anything was called, use `call()`,
below, or ask first with `empty()` or the conversion to `bool`.

`operator()` is `const`, as in `std::function`, and what is held may
change itself when it is called (a lambda that is `mutable`, say).


## Copying and moving

A `cobfunction` can be copied, and the copy holds a copy of what the
original holds. It has no move operations: a "move" copies, and what
was "moved from" still holds what it held. (A `std::function` that
was moved from is left in a state that is not specified.)

A `cobmovefunction` can only be moved, and so it can hold also what
can only be moved - say, a lambda that captured a `std::unique_ptr`.
What it was moved from holds nothing.

Two functions of different `BYTES` are different types and do not
convert to one another. One can be put into the other like any other
callable, if it fits.


# What is added


## Assign what may not fit

```cpp
template <class F> bool assign(F&& f);
```

Makes this hold `f`, and returns `true`. If `f` does not fit, returns
`false`, and this holds what it held before.

This is for when the size is not known to be right at compile time:
in a template, or for an `atbfunction`, where it is the only way to
put something in. What can't be called the right way still does not
compile.

If `f` is a null pointer to a function, or to a member, this is left
holding nothing, and `true` is returned.


## Call, and tell whether it did

```cpp
bool call(A... a) const;                                    // if R is void
template <template <class> class V> V<R> call(A... a) const;
```

Like `operator()`, but the result tells whether there was something
to call.

When `R` is `void`, `call()` returns `true` if it called, and `false`
if this holds nothing.

Otherwise `call<V>()` returns the result in a generic optional (see
[common.md](common.md)), or nothing if this holds nothing:

```cpp
cobfunction<int(int)> f;
auto r = f.call<std::optional>(5);      // nothing
f = [](int x) { return x + 1; };
r = f.call<std::optional>(5);           // 6
```


## Is it empty, and making it so

```cpp
explicit operator bool() const noexcept;
bool empty() const noexcept;
void clear();
```

The conversion to `bool` is as in the Standard: `true` if it holds
something. `empty()` is the opposite of it.

`clear()` destroys what is held, leaving this holding nothing.


## The room

```cpp
static auto capacity();
template <class D> static constexpr bool takes;
```

`capacity()` is `BYTES`: how big what it holds can be.

`takes<D>` tells whether a `D` could be held as far as its type goes:
whether it can be called the right way and copied (or moved). It does
not look at the size.


# What it takes

`BYTES`, rounded up to the alignment of `std::max_align_t`, and a
pointer.
