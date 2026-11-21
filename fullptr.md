# Pointers

```cpp
#include "fullptr.hpp"          fullptr<T>
#include "optptr.hpp"           optptr<T>
#include "fullshared.hpp"       fullshared<T>
#include "optshared.hpp"        optshared<T>
#include "fullunique.hpp"       fullunique<T>
#include "optunique.hpp"        optunique<T>
```

Dereferencing a null pointer is undefined behaviour. These types
remove the chance of it, in one of two ways:

* A `full` pointer is never null. It can be dereferenced freely.
* An `opt` pointer may be null. It can't be dereferenced at all: the
  object is reached only through a function that is called if there
  is an object.

There are three pairs, by who owns the object:

| Ownership             | Never null    | May be null  |
|-----------------------|---------------|--------------|
| none (a plain pointer)| `fullptr`     | `optptr`     |
| shared                | `fullshared`  | `optshared`  |
| sole                  | `fullunique`  | `optunique`  |

There is no Standard counterpart of any of them. The shared and
unique ones are built on `std::shared_ptr` and `std::unique_ptr`, and
can hand those over to code that wants them.

What none of them can help with is a pointer to an object that is
gone. `fullptr` and `optptr` own nothing, so that is as much up to you
as with a plain pointer.


# `fullptr`: a pointer that is never null

A `fullptr<T>` is the size of a `T*`, and is used like one. It can't
be made from a pointer that is null, so it never is null, and that is
known from its type: a function that takes a `fullptr<T>` need not
check, and can't forget to.

There is no default constructor, as there would be nothing for it to
point to. Everything can be used in constant expressions.


## Make one

```cpp
template <template <class> class V> static constexpr V<fullptr> make(T* p);
static constexpr fullptr from(T& r);

template <template <class> class V, class T> constexpr V<fullptr<T>> make_fullptr(T* p);
template <class T> perhaps<fullptr<T>> make_fullptr(T* p);
```

`make<V>(p)` returns a `fullptr` to what `p` points to, in a generic
optional (see [common.md](common.md)); nothing if `p` is null. This is
where the check is, once.

`from(r)` makes one from a reference. A reference always refers to an
object, so there is nothing to check and the result is not optional.

`make_fullptr<V>(p)` is `make<V>(p)` with `T` worked out from `p`, and
`make_fullptr(p)` is that with `perhaps` as the optional.

```cpp
int  x = 1;
auto p = fullptr<int>::from(x);
auto q = make_fullptr(find(key));           // perhaps<fullptr<item>>
q.and_then([](fullptr<item> i) { i->touch(); });
```

```cpp
template <class U> constexpr fullptr(fullptr<U> x);
```

A `fullptr<U>` converts to a `fullptr<T>` whenever a `U*` converts to
a `T*`: to a pointer to `const`, or to a base class.


## Use it

```cpp
constexpr T&       operator*();
constexpr T const& operator*() const;
constexpr T*       operator->();
constexpr T const* operator->() const;
constexpr T*       get();
constexpr T const* get() const;
constexpr bool     operator==(fullptr const&) const;
```

As a pointer. `get()` is the plain pointer, which is not null. Two
are equal if they point to the same object.

Note that a `const fullptr<T>` gives access to `const` only, unlike a
`T* const`.


# `optptr`: a pointer that may be null

An `optptr<T>` is the size of a `T*`: null itself is what says that
there is no object. It is what `perhaps<fullptr<T>>` is, without the
flag. Everything can be used in constant expressions.


## Make one

```cpp
constexpr optptr(T* x);
constexpr optptr(std::nullptr_t);
constexpr optptr(fullptr<T> x);
```

From any pointer, null or not, and from a `fullptr`. The constructors
are not `explicit`, so a function that returns an `optptr<T>` can just
return a pointer. There is no default constructor.


## Is there an object

```cpp
constexpr explicit operator bool() const;
```

`true` if it is not null.


## Use the object

```cpp
template <class F> constexpr auto and_then(F&& f);
template <class F> constexpr auto and_then(F&& f) const;
```

Calls `f` with a reference to the object, if there is one: a
reference to `const` if the `optptr` is `const`. If `f` returns
`void`, so does `and_then()`. Otherwise it returns what `f` returned,
or a value initialised object of that type if there is no object.

This is the only way to the object. There is no `*`, no `->` and no
`get()`.

```cpp
optptr<node> next = n.next();
next.and_then([](node& x) { x.visit(); });
int depth = next.and_then([](node& x) { return x.depth(); });   // 0 if there is none
```

```cpp
constexpr fullptr<T> value_or(fullptr<T> x) const;
template <class F> constexpr auto or_else(F&& f);
template <class F> constexpr auto or_else(F&& f) const;
```

`value_or(x)` returns a `fullptr` to the object, or `x` if there is
none. It is the way to get a pointer that can be dereferenced: by
saying what to point to otherwise.

`or_else(f)` returns this if it is not null, and `f()` otherwise; `f`
should return an `optptr<T>`.


## Make it null

```cpp
constexpr void reset();
constexpr auto release();
```

`reset()` makes it null. `release()` does too, and returns the plain
pointer that it held, which may be null. That is the way out to code
that wants a `T*`; nothing is owned, so nothing is deleted by either.


# `fullshared`: shared ownership, never null

A `std::shared_ptr<T>` that is known not to be null.


## Make one

```cpp
template <class... A> static fullshared make(A&&... a);
template <template <class> class V> static V<fullshared> from(std::shared_ptr<T> x);
```

`make(a...)` makes a new object from `a...`, as `std::make_shared`
does. If there is no memory for it, `std::bad_alloc` is thrown, as by
any `new`.

`from<V>(x)` takes a `std::shared_ptr` you have: returns a
`fullshared` in a generic optional, or nothing if `x` is null.

There is no default constructor.


## Copy, but not move

A `fullshared` can be copied; the copies share the object, as with
`std::shared_ptr`.

It has no move operations, as what is moved from would be left null.
A "move" copies, and what was "moved from" is as good as before.


## Use it

```cpp
T& operator*() const;
T* operator->() const;
std::shared_ptr<T> shared() const;
bool operator==(fullshared const&) const;
```

As a pointer. `shared()` returns a `std::shared_ptr` that shares the
object, to hand over to code that wants one; it is not null. Two are
equal if they share the same object.

```cpp
auto a = fullshared<std::string>::make("abc");
a->append("d");
```


# `optshared`: shared ownership, may be null

A `std::shared_ptr<T>` that can't be dereferenced.


## Make one

```cpp
optshared(std::shared_ptr<T> x);
optshared(std::nullptr_t);
optshared(fullshared<T> x);
```

From a `std::shared_ptr`, null or not, from `nullptr` and from a
`fullshared`. There is no default constructor. It can be copied and
moved; one that was moved from is null.


## Use the object

```cpp
explicit operator bool() const;
template <class F> auto and_then(F&& f) const;
```

The conversion to `bool` tells whether there is an object.

`and_then(f)` calls `f` with a reference to the object, if there is
one, and returns as `and_then()` of `optptr` does. It is the only way
to the object. Note that `f` gets a `T&` also when the `optshared` is
`const`, as with `std::shared_ptr`.

```cpp
optshared<std::string> o = a;
o.and_then([](std::string& s) { s += "e"; });
```


## Get a pointer that is not null

```cpp
template <template <class> class V> V<fullshared<T>> full() const;
fullshared<T> value_or(fullshared<T> x) const;
template <class F> auto or_else(F&& f) const;
```

`full<V>()` returns a `fullshared` to the object in a generic
optional, or nothing if this is null.

`value_or(x)` returns a `fullshared` to the object, or `x` if this is
null.

`or_else(f)` returns this if it is not null, and `f()` otherwise; `f`
should return an `optshared<T>`.


## Other members

```cpp
void reset();
std::shared_ptr<T> shared() const;
```

`reset()` makes it null, giving up its share of the object.
`shared()` returns the `std::shared_ptr`, which may be null.


# `fullunique`: sole ownership, never null

An object on the heap that is destroyed with its `fullunique`.

```cpp
template <class... A> static fullunique make(A&&... a);
T&       operator*();
T const& operator*() const;
T*       operator->();
T const* operator->() const;
```

`make(a...)` makes a new object from `a...`, as `std::make_unique`
does (and throws `std::bad_alloc` if there is no memory). That is the
only way to make one.

It can be neither copied nor moved: copying would make two owners,
and moving would leave a null behind. So a `fullunique` stays where
it was made - as a local variable, or a member - and what `make()`
returns can only be used to initialise one. A `const fullunique` gives
access to `const` only.

```cpp
auto buf = fullunique<buffer>::make(4096);
buf->fill(0);
```

If the object has to change hands, use `optunique`.


# `optunique`: sole ownership, may be null

A `std::unique_ptr<T>` that can't be dereferenced.


## Make one

```cpp
template <class... A> static optunique make(A&&... a);
optunique(std::unique_ptr<T>&& x);
optunique(std::nullptr_t);
```

`make(a...)` makes a new object, as `std::make_unique` does. One can
also be made from a `std::unique_ptr`, which is moved from, and from
`nullptr`. There is no default constructor.

It can be moved, not copied. One that was moved from is null - which
is a state like any other for it.


## Use the object

```cpp
explicit operator bool() const;
template <class F> auto and_then(F&& f);
template <class F> auto and_then(F&& f) const;
```

The conversion to `bool` tells whether there is an object.

`and_then(f)` calls `f` with a reference to the object, if there is
one - to `const`, if the `optunique` is `const` - and returns as
`and_then()` of `optptr` does. It is the only way to the object.


## Give the object up

```cpp
void reset();
std::unique_ptr<T> release();
```

`reset()` destroys the object and makes this null.

`release()` returns the `std::unique_ptr`, and with it the ownership,
leaving this null. Note that this is not what `release()` of
`std::unique_ptr` is: that one returns a plain pointer that nobody
owns.
