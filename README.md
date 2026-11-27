# Compile or allocation time bound, and thus safe, types

For description in Serbian, see: [Опис на српском](README.sr.md).

This is a collection of types that are statically - at compile time,
or at allocation time bound, so that you can't accidentally misuse
them and get undefined behavior. They are, thus safe. This is C++, you
can still cast away all this safety or do other nasty things, but,
there's nothing like "what if my integer addition overflows" or "what
if I call front() on an empty std::vector". Also, there's no dreaded
"should I throw or return an error if I detect invalid operation at
runtime".

There are:

* Range bound integers. Like in Ada. Beside being statically bound
  (say, from 0 to 5), they also cannot overflow (more like Spark/Ada).
* Dimension bound arrays. Similar to `std::array<>`, but, indexing is checked
  to be in range at compile time (with range bound integers) and there are no
  potentially unsafe interfaces. Invalid iterators point to a "safety spot", so 
  if someone dereferences them on purpose, there's no undefined behavior. You'll 
  still get _incorrect_ behavior, but, it's safe, not _undefined_. The only
  undefined behavior left are iterators thet outlive their arrays, which is a
  design flaw in STL, which there is no easy way around, and without support for
  STL iterators, the whole `<algorithm>` and range-for would not be available.
  So, don't ever use iterators on your own, but just to pass to standard
  algorithms or implicitly in range-for, and you should be good.
  The iterators satisfy the C++20 iterator concepts (random access for arrays
  and matrices, bidirectional for `coblist`, forward for the rest), so
  `std::ranges` algorithms work: `std::ranges::sort(a)`. A default-constructed
  iterator points to the end of a per-thread dummy container, so using it is
  safe too. Elements of a `cobskiplist` are read-only through iterators,
  since changing a key would break the ordering.
* Dimension bound matrices - two dimensional arrays. Needed to avoid copying
  whole rows on accessing elements.
* Pointers that are never null(ptr). We call them `fullptr<T>`. While there are
  similar implementations that check that pointer is not `nullptr` at runtime,
  here we prevent constructing a `fullptr<>` from a pointer that is `nullptr`. 
  While that prevention is done at runtime, the `fullptr<>` is known at compile
  time to _never_ be null. They have no notion of ownership and you can delete
  the "underlying" pointer if you wish.
* An optional value with no unsafe access. We call it `perhaps<T>` and it's like
  `std::optional<>` with just the "monadic" and other safe functions. It's also
  much simpler than usual "lite" optional types. which means faster compilation
  but potentially more edge cases. We also do not consider returning something
  other than `perhaps<T>` from the function passed to `and_then()` and `or_else()`,
  as an error, though if you do so, you won't get the "monadic" behaviour.
  But it's convenient - see `join` in `perhaps.t.cpp`.
* A safe pointer, which can be `nullptr`, but if it is, it cannot be accessed.
  We call it `optptr` (optional pointer, of sorts).
  It's similar to `perhaps<fullptr<T>>`, but it doesn't need a separate boolean
  indicator to see if there is a valid pointer; it uses the `nullptr` as a
  sentinel value. Thus, it takes as much memory as `T*`. It's also much simpler
  which means faster compilation, nicer compiler error messages and less potential
  edge cases in use.
* Capacity bound lists. Interface is similar to `std::list`/`std::forward_list`, 
  but, their capacity is fixed at compile time. So, inserting can fail (no more 
  room). Also, the "pool" of elements is allocated in an array "up front", 
  quite unlike `std::list`. On the flip side, the iterators, even invalidated, 
  can always be dereferenced while the list is live, although they might not be 
  in the list at that time (there's a member function to check that, but it's
  slow, so you have to call it explicitly). So, you may get unspecified, but 
  not undefined behavior. As for iterators outliving their lists, there are 
  interfaces which use indeces rather than iterators, which are safe and you 
  should use them when not dealing with STL.
* Capacity bound skip lists. They model the `std::set<>` interface, and
  `cobmap` is a key-value view of them, modelling `std::map<>`.
  If the need arises, should be simple to add `std::multiset<>`
  and `std::multimap<>` support. Other remarks are similar to general capacity 
  bound lists above.
* Capacity bound Timer lists. Very simple and flexible. One can start
  a timer - O(N), stop it by ID - O(N), stop by index gotten when
  starting - O(1) and process the ones that have expired - O(1).
* Capacity bound Timer list wheel. An optimization with a number of
  "spokes" each being a timer list and used for timers that expire
  "modulo spokes". Thus inserting and searching is "spokes times
  faster". It loses some flexibility as it now has to be based on
  "ticks", the durations cannot be arbitrary any more. There is no
  loss in precision: a timer goes to the spoke that will be checked
  on the very tick the timer is to expire on.
* Capacity bound Timer mill. It has several wheels, but each spoke is
  now a simple "vector" of timers. Each wheel has a different scale of
  ticks, thus longer timers lose more precision. Also there is a limit
  for possible max duration. These issues are compensated by insertion
  now being O(1). There are two variants - regular and lean. For
  regular, each spoke is a doubly linked list. This can be a large
  footprint, so, if you need to conserve memory, use the lean mill.
  It's also a little slower, but, it has a drawback - when a timer
  is cancelled, it's not actually removed from the list, it's just
  "nulified". That is, you need to have a way to distinguies this
  "cancelled" timer from a "non-cancelled one".
  The regular mill has a compile time option to be exact, by cascading
  the timers from wheel to wheel, so that no precision is lost.
* Capacity bound Timer lean ring. For where memory is tight and timers
  mostly expire (are rarely cancelled). The timers are kept sorted by
  expiry in a ring (a circular array) with no links, no pool and no
  per-timer bookkeeping: the only room beyond the IDs is the expiry
  time of each, and the head and tail of the ring. There is no stop by
  index (inserting moves timers around); cancelling just "nulifies" a
  timer in place, leaving a hole that a later start reuses. Unlike the
  lean mill, a cancelled timer is never reported. Starting is O(N),
  stopping the first timer and processing expiries are O(1) per timer.
* Elements of any type in the lists and the skip list (`std::string`, `std::unique_ptr`, types
  without a default constructor), constructed and destroyed exactly when inserted and erased.
* `contingent<T>`: an optional that uses a sentinel value (nullptr, NaN, a chosen integer, the
  integer just outside a range), so it takes no more room than the value.
* `fullshared`/`optshared` and `fullunique`/`optunique`: shared and unique pointers that are
  never null, or that can be null but can't be accessed without checking.
* `atbarray`: an array whose dimension is set once at run time, yet whose indices are checked
  at compile time (the `atb` prefix is for _allocation time bound_ containers).
* `cobpool` (object pool), `cobring` (ring buffer), `cobbitset` and `cobhash` (hash table).
* `cobflatset` and `cobflatmap`: a set and a map kept in order in a vector, with nothing but the
  elements in them. For what is small, or mostly looked up.
* `cobhive`: the pool that is good at going over its objects.
* `cobtree`: a balanced (AVL) search tree, with the interface of the skiplist and a guaranteed
  O(log n) for everything. `cobtreemap` is the map on it. AVL is chosen over the red-black
  tree because it's more often used in real-time applications; both are bad at insertion and
  deletion, so the real-time use avoids them anyway yet AVL gives a better guarantee of
  tree depth, so lookups are better time-bound.
* `cobspsc`: a queue between two threads, with no locks and no waiting. Each of its two sides
  is given out once.
* Lists, skiplists, trees, maps, pools and timers each have a _checked_ variant
  (`cobcheckedlist`, `cobcheckedtmrmill`...): the link of an erased element, or the index of a
  timer that is not running, is then never taken for that of a new one. The lean timer ring has
  no index, so no checked variant.
* The same ones, and the cache, each have a _hive_ variant (`cobhivelist`, `cobhivetmrmill`,
  `cobcheckedhivetree`...), which keeps a bit, not a byte, for each slot to tell if it is in use.
* `cobfunction`: like `std::function`, but what it holds is in the object, so it never allocates.
  `cobmovefunction` is the one that can only be moved.
* `coblru`: a cache that drops what was not used for the longest time.
* `cobvector`: an array that grows up to its capacity. `cobbinheap`: a binary heap (priority
  queue). `cobmap`: keys and their values, ordered by the keys.
* `cobstring`: a string of at most 255 characters with the interface of `std::string`, less what
  is unsafe. It takes one character more than its capacity.
* An `atb` variant of each container: `atblist`, `atbfwdlist`, `atbskiplist`, `atbmap`, `atbpool`,
  `atbring`, `atbbitset`, `atbhash`, `atbmatrix`, `atbvector`, `atbbinheap`, `atbstring`, `atbtree`,
  `atbtreemap`, `atbspsc`, `atbfunction` and `atblru`, and of each timer.

This should provide, with the caveats mentioned above, the same level
of safety as, say, Ada/Spark's "no runtime errors". It's actually
higher safety than most other languages, including various "safe"
ones, such as "plain" Ada, that catch such errors at runtime.

The library has reached a level of completeness, as most types that I
envisioned implemented have been implemented and most types from C++
standard library have their equivalents. Other types might be added in
the future, but there will probably have to be a pretty good reason for that.

## Getting started

Just copy the headers you need where you'd like them to be and include them.

This is designed for and tested with C++20, though some headers require only
C++17.

## Usage

Should be rather simple. Look at the [bounds test](bound.t.cpp) and
other tests for examples. Read the docs if you need more info. Since
this is a header-only library, you only need to set the include
directories.

If you're using the `meson` build tool, there's a `meson.build` so that you can use this as
a meson subproject, which should take care of include directories.

## Documentation

This file is the introduction: what there is, why, and how it is meant to be used. The reference
for each class, or group of classes, is in a file of its own. Start with [common.md](common.md),
which has what all the containers share (and the list of all the files):

* [cobi.md](cobi.md): range bound integers. [atbdim.md](atbdim.md): dimensions set at run time,
  and what is different about the `atb` containers.
* [cobarray.md](cobarray.md), [cobmatrix.md](cobmatrix.md), [cobvector.md](cobvector.md),
  [cobstring.md](cobstring.md), [cobbitset.md](cobbitset.md).
* [cobpool.md](cobpool.md) (pools and hives), [coblist.md](coblist.md) (both lists),
  [cobskiplist.md](cobskiplist.md) (the skiplist and the tree), [cobmap.md](cobmap.md) (the maps
  on those two), [cobflat.md](cobflat.md) (the flat set and map), [cobhash.md](cobhash.md),
  [coblru.md](coblru.md), [cobbinheap.md](cobbinheap.md), [cobring.md](cobring.md).
* [cobspsc.md](cobspsc.md) (the queue between two threads), [cobfunction.md](cobfunction.md).
* [perhaps.md](perhaps.md) (`perhaps` and `contingent`), [fullptr.md](fullptr.md) (all the
  pointers).
* [timers.md](timers.md).

Where a class mimics one of the Standard library, its file says only how it differs from that
one. Where there is no Standard counterpart, all of it is described.

## Basic design decisions

Using templates and constant expressions, check for bounds at compile time.

To the extent possible, make it so that the compiled code is essentially the
same as the code you would get if you wrote "optimal safe code", that is code
with the least needed amount of checks.

We use the C++ constant expression machinery which implies that there can be
no undefined behavior in constant expressions, as to prevent integer overflow and
similar nasty things.

Also, we're aiming for smallest amount of code, both for simplicity, ease of
maintenance and for higher assurance of correctness, as this is rather fundamental
code - if you do rely on it for safety, it'd better be right. This also implies a
minimalistic feature set. For example, it's highly unlikely we will ever add a
"big integer" type.

For example, there's [bounded-integer](https://github.com/davidstone/bounded-integer),
which is a 900 pound gorrila with just about anything you could think of. But, it's a
lot of code and one probably needs only a portion thereof in most cases. 
There are some others, for example, [safe-arithmetic](https://github.com/intel/safe-arithmetic)
has the same general aim as we do, but is more narrow-focused on arithmetics and is much
larger with lots of features that are less frequently used in general 
(also, at the time of this writing is in "pre-release work-in-progress" stage).

Each type is in a header of it's own, though it might include other headers,
as needed. While being "header-only" is not a goal, so far all the types are
header-only and it's rather likely that all will be, as we rely on templates
and constant expressions (`constexpr` and the like).


## Range bound integers

Say you're writing software for a computer that has 6 USB ports and you're
somehow controlling these ports. Other examples are data about minutes, hours
and seconds (leap seconds notwithstanding) of a clock, apartments in a building,
age of high-school children, etc.

Back to our USB ports, if you want to index one of them, you can define:

```cpp
    using usbi = cobi<int, 0, 5>;
```

Now, whenever you use `usbi`, you will be sure, at compile time, that it's value
is within this range. The only way to get a value "inside" `usbi` is to call it's
`be()` member function, which will check the range and return an error if it's not
within range, or to assign from another `cobi<int>` which is in same or narrower
range as `usbi`.

Besides that, you can actually use `cobi`s to do arithmetics for arbitrary ranges
and you'l end up with a value in some final range. Then you can just assign it to
the one you'd like - if it compiles, great! If not, then use `be()` and handle the
"out of bounds" case. You just compute as you wish, and we'll track the range of
the computation for you.

This is OK:

```cpp
    cobi<int, 0, 2> x{};
    cobi<int, 2, 3> y{};
    usbi z = x + y;
```

but this is not:

```cpp
    cobi<int, 0, 1> x{};
    usbi y{};
    usbi z = x + y; //!! x+y range is <0,6> and 6 > 5
```

and this is just asking for it;

```cpp
    usbi x{};
    usbi y{};
    usbi z = x + y; //!! final range is <0,10>!
```

This is a little strange at first, but, think about it, if `x == 5` _and_
`y == 5`, then `x + y == 10`, which is clearly larger than `5`, and we can't
have that.

We're used to adding integers with no checks, but, as we know, that can overflow, it's just
that we don't think much about it. With `cobi`, we still don't have to:

```cpp
    cobi<int, std::numeric_limits<int>:max() - 1> x;
    cobi<int, 1> x;
    auto z = x + y;
```

For convenience, the upper bound is by default the same as the lower. So, there's no
overflow for `z`, but:

```cpp
    auto w = z + x;
```

There it is! Even though we're using `auto` and thus "accepting" whatever
range of `z + x` might be, since `z + x` overflows, that would be undefined behavior,
and we can't have that.

### Iterate over the whole range

If you know Ada, you know the nice syntax:

```ada
    for i in x'range loop
```

There are no _such_ attributes in C++, but there is 
range-for, so we provide a `range()` function. 

```cpp
    cobi<WHATEV, LOWER, UPPER> x;
    for (auto i: x.range()) {
        std::cout << i; // or, whatever...
    }
```

The `range()` is actually a static function, so, if
you have an alias and don't have a variable:

```cpp
    for (auto i: usbi::range()) {
        std::cout << i ;
    }
```

This is valid iterator-pair range, so numeric algorithms
should work fine:

```cpp
   auto r = std::accumulate(x.range().begin(), x.range.end(), 0);
```

There's one downside, this uses one integer as the `end()`
iterator mark, so, you can't have the full range.

```cpp
    cobint<0, std::numeric_limits<int>::max> x;
    auto r = x.range(); // !!! can't have that
```

If you don't want to involve iterators, ranges and such,
or just need the bound to be the maximum:

```cpp
cobint<LOWER, UPPER> i;
do {
    // use `i` here
} while (i.advance());
```

So, it's a little verbose, but, not oherwise weird. It relies on
constructor setting the value to the lower bound.

To go backwards:

```cpp
auto i = cobint<LOWER, UPPER>::greatest();
do {
    // use `i` here
} while (i.ebb());
```


### More things to do with range bound integers

```cpp
cobint<0, 9> x = cobint<0, 9>::clamp(99);     // 9: the nearest value of the range
x.advance_wrap();                              // 0: goes around, instead of failing
auto m = max(x, cobint<3, 4>{});               // cobint<3, 9>
auto a = abs(cobint<-5, 3>{});                 // cobint<0, 5>
auto w = cobint<0, 5>{}.as<long long>() + cobi<long long, 1LL << 40>{};  // mixing types
std::cout << x << std::endl;                   // prints the number
```

The three-way comparison `<=>` works between ranges and with plain integers. Conversion
between ranges is allowed only to a wider range, and `std::is_convertible` tells the truth.

### Compiler error messages

TLDR; they're not great.

Note that converting to a narrower range is rejected by the constructor constraints, so
the message is "no viable conversion" (or similar), rather than "Out of upper bounds".

So, compilers should catch undefined behavior at compile time. In our case, this is
mostly overflow, but could be other things, such as division by zero. The ones we tried
(GCC, Clang, MSVC) mostly do, but not always. When they do, the
errors reported are not great. Unfortunately, there's nothing we can do about it,
except complain.

In general it's like this:

* GCC is the most verbose one. Sometimes in all the details you can find enough
  information to figure out what's going on and how to fix it.
* Clang tries to be smart about this. Error reports are much shorter and, when it
  hits, it's great. But, more often than not, it misses.
* MSVC tries to be more like Clang, though it's not always. At times, the reports
  are just bad, with almost no clue what's going on.

### Nicer error messages for MSVC - by error

Microsoft Visual C++ compiler, at the time of this writing in January 2023,
does not actually catch all overflows at compile time in constant expressions.
So, you'll find a few places in the code that have `#ifdef _MSC_VER` and do the
overflow checks. If MSVC becomes more standards-compliant in the future, this
can safely be deleted.

A nice side-effect is that in these cases you will get nicer error messages than
the usual MSVC messages.

### Nicer error messages - if only

If you're not happy with the compiler error messages for overflow, you
might be tempted to somehow circumvent that. For some, you can turn
these checks off in GCC with `-fpermissive`.  Or, you might use some
compiler other than the ones we did, and it might not have these
checks.

In such cases, following remarks might come in handy.
For some odd reason, they all pertain to the minus operators.

In the body of the operator unary minus, the following code would make for nicer error
messages in catching the inability to get the oposite of the minumum possible
negative value. Of course, this assumes executing on a computer that uses 
two's complement integers - which is essentially all computers in 2023.

```cpp
    static_assert(D > std::numeric_limits<T>::lowest(), "Negation Overflow");
    static_assert(G > std::numeric_limits<T>::lowest(), "Negation Overflow");
```

In the body of the binary minus, we could implement it on its own, rather than
delegating to operator plus. It would go like this:

```cpp
    if constexpr (D2 == std::numeric_limits<T>::lowest()) {
        static_assert(G1 > 0);
    }
    else {
        constexpr auto upperG = std::max(G1, -D2);
        constexpr auto lowerG = std::min(G1, -D2);
        if constexpr (upperG >= 0) {
            static_assert(std::numeric_limits<T>::max() - upperG >= lowerG);
        }
    }

    if constexpr (G2 == std::numeric_limits<T>::lowest()) {
        static_assert(D1 > 0);
    }
    else {
        constexpr auto upperD = std::max(D1, -G2);
        constexpr auto lowerD = std::min(D1, -G2);
        static_assert(std::numeric_limits<T>::lowest() - lowerD <= upperD);
    }
    cobi<T, D1-G2, G1-D2> r;
    r.i = x.i - y.i;
    return r;
```

### Division by zero

It's a little strange, but, if one of the bounds of the divisor is zero, then compiler
would catch that and you'll get a not-so-great error message (though, they're good-enough
in our experience). If the bound itself is not zero, then you will run into the `static_assert` 
if the _range_ of the divisor includes zero. That is, compilation will fail if there's a 
_chance_ that the divisor might be zero.

Sure, it's very strict, but that's kind of the point. If you want to add a runtime check,
that's fine, but how do you do it, depends on your situation. See, division is interesting
as the only other problem is if the divisior is `-1` and dividend is `std::numeric_limits<>::lowest()`.
So, you could (`LOWER < 0` and  `UPPER > 0`):

```cpp
    cobi<int, LOWER, UPPER> x;
    cobi<int, LOWER, UPPER> y;
    cobi<int, 1, UPPER> posdiv;
    cobi<int, LOWER, -1> negdiv; //
    if (posdiv.be(y.get())) {
        auto r = x / posdiv;
    }
    else if (negdiv.be(y.get())) {
        auto r = x / negdiv;
    }
```

There are other ways, that is you can simplify this to some extent depending on your context, 
but, it's really tedious in any case. You will try to avoid mixed-sign bounds for the divisor.

If you're compiling:
1. for a processor that gives the result of `0` for division by zero
2. with a compiler that knows this and doesn't treat division by zero as anything special
3. with no care of portability 

then you can just remove the "possible division by zero" check in the
`cobi` code and be happy (also, wonder why don't all processors do
this?).

### There's no bitwise operators

We're modelling mathematic numbers here, there's no "bitwise operators" in mathematics.

Besides, these are weird about the range of the result. Let's just look at one example - bitwise
AND. Let's say operands are

```cpp
    cobi<int,33> x;
    cobi<int,33> y;
    cobi<int,16> z;
    cobi<int,16,17> w;
```

Assuming that we had bitwise and operator, this illustrates the range of the result:

```cpp
    cobi<int, 33, 33> r  = x & y;
    cobi<int,  0,  0> r  = x & z;
    cobi<int,  0,  1> r  = x & w;
```

It's actually non-trivial to come up with the final range and we just never had 
a need for this.

The [safe-arithmetic](https://github.com/intel/safe-arithmetic) provides bitwise
operations by having a different check - a bitmask based one. In our design this
requires a different type, which might be added in the future. But `cobi` itself
will probably never "acquire" bitwise operators.

### Comparison might be done at compile time

If you have two integers of distinct ranges:

```cpp
    cobi<int, -5, -1> x;
    cobi<int, 0, 5> y;
```

then comparing them might be done at compile time:

```cpp
    constexpr bool same = x == y;
```

That is `same` is a compile time constant equaling `false`, which might come in handy in
certain situations.


### Most integers are ints

For those, we provide a shortcut: `cobint<0,5>` is the same as `cobi<int, 0,5>`.

This is needed because we can't have `int` be a default type and
define a range (at best we could have the default range of `0,0` for the default `int` type). 
For constants, there's `cobic`, which can save a few keystrokes and looks nicer in expressions
(it's similar to `std::integral_constant<>`, but again saves keystrokes):

```cpp
    cobint<0,5> x;
    auto r = x + cobint<8>{};
    auto q = x + cobic<8>;
```


## Range bound arrays

Other than using Range Bound Integers for the index, thus knowing at compile time that there's no
out of array bounds access, we also don't follow the STL interface "to the letter".

We don't give the reference to any member and provide three access interfaces:

* `get(index)` returns a _copy_ at the given index. While this may be a performance
penalty it is also safe against a common `std::array<>` problem (simplified
for illustration here):

```cpp
    class X {
        int& f() {
            return a[1];
        }
        std::array<int, 3> a;
    };

    int& r = X().f();
    r = 5; // !!! writing to a dangling reference
```

   Also, for structs/classes we provide a helper `get(index, data-member-pointer)`
   which will get only the given member. It's a little cumbersome, but works well.
* `set(index, value)` will set the element to the given value, which implies a copy and again
   could be a performance penalty, and we also provide `set(index, data-member-pointer, value)`.
* `grab(index)` returns a _pr-only wrapper_ with an overloaded `operator->`. You cannot get
   its address, or even copy it. It's sole purpose is to provide a nicer, yet safe, syntax
   for accessing or changing members of class/struct and it's unusable for scalar types.

To illustrate:
```cpp
    struct S {
        int i;
        float f = 0.0;
    };
    cobarray<S, 3> as;
    as.set(cobic<0>, &S::f, 2.0f);
    assert(2.0 == as.get(cobic<0>, &S::f));
    as.grab(cobic<0>)->f = 4.0;
    assert(4.0 == as.grab(cobic<0>)->f);
```

For safety, we allocate an extra element as the "landing zone" for the out of
bounds iterator. If you push the iterator out of array bounds, it will "sit" in
the landing zone and dereferencing it will give you "something" _well defined_,
we just don't care what it is (you could say it's "unspecified"). Unfortunately,
this safety comes with a price of extra checks in the iterator.

A safe _and_ fast way to iterage through the array is to use a helper range iterator, like:

```cpp
    cobarray<WHATEV, DIM> a;
    for (auto i: a.irange()) {
        a.set(i, SOME);
        a.set(i, &WHATEV::member, THING);
        a.grab(i)->member = THING;       // if WHATEV is a struct/class
        std::cout << a.get(i);
        std::cout << a.grab(i)->member;  // if WHATEV is a struct/class
    }
```

One thing, though - all the elements of an array are there all the time, and they are value
initialised when the array is made: numbers are 0 and the default constructor is called for
each object. Arrays can be very big, so be mindful of what you put into this array. (A
`cobmatrix` does not set numbers to 0: fill it before reading from it.)


## Range bound lists

Lists keep their elements in a `cobpool`, which maintains a helper stack of free elements. The
next (and previous) "pointers" are actually indeces to said elements, and are kept in helper
arrays of indeces. The same pool is what the skiplists (and thus the maps) and the exact timer
mill keep their elements in, so that making, dropping and telling if a slot is live is done in
one place only. What the two lists have in common otherwise is in `cobchain.hpp`.

Like for arrays, we have one more element than "asked for" and the "end()" iterator points 
to it. We use its index as the equivivalent of a `nullptr` from the usual "dynamically
allocated lists".

### Usual members

Lists keep an element count, so `size()` is O(1), and have `full()`, `front<V>()` (and
`back<V>()` for `coblist`), which give a copy in a "generic optional" `V`. `coblist` has
`splice(pos, other)`, `merge(other[, comp])` and `swap(other)`; `cobfwdlist` has `merge` and
`swap`. They return `false` and change nothing if the elements would not all fit. `sort()` is
a stable merge sort, O(n log n), which only changes links. `cobskiplist` has `upper_bound`.
Arrays and matrices have `==`, `front()`, `back()` and `swap()`; matrices also `rows()`,
`columns()`, `get_row(r)` and `set_row(r, v)`. `perhaps` has `emplace()` and `==`.
`fullptr<T>::from(ref)` makes a `fullptr` from a reference, and a `fullptr` converts to
a `fullptr` of a const or base type. `optptr::value_or` returns a `fullptr`, and `and_then`
passes a reference rather than a pointer.

### Elements of any type

The lists and the skip list hold any type, not just PODs: `std::string`, `std::unique_ptr<X>`,
types without a default constructor. Elements are constructed when inserted and destroyed
when erased (`emplace_front`, `emplace_back`, `emplace`, `emplace_after`). Because a type
may not be copyable, there is `with(link, f)`, which calls `f` with the element if there is
one, and `for_each(f)`:

```cpp
coblist<std::unique_ptr<int>, 4> l;
l.emplace_back(new int(2));
l.with(l.lfront(), [](std::unique_ptr<int>& p) { *p = 3; });
```

`get(link)` returns a copy (or `T{}` when there is no element), so it needs a default
constructor. Dereferencing an iterator that points to no element gives a per-thread
"spot" object, which also needs a default constructor, but only if you dereference such
iterators. The library assumes that copying an element does not throw.

Arrays and matrices keep all elements alive all the time, so the element needs a default
constructor. They have `cobarray<int, 3> a{1, 2, 3}` (exactly N values, checked at compile
time), `filled(v)` and `generate(f)`, with `f` called with each index.

The interface of `std::list` is mostly there and behaves similarly, but those functions that
use iterators should not be used unless you have to. Use the alternate/mimicked functions
which accept the indeces, which are safe. To check whether there is an element at a link
(for example, a link kept after the element was erased), use `live(link)` (or `contains()`),
which is O(1). A stale link is harmless: `erase`, `insert` and `set` do nothing with it. Of
course, if a new element reused the slot, the link refers to that new element.

If you take a look at the code, you will notice that there may be many problems in the code
which cannot be addressed with our compile-time bound integers and arrays. The forward
and backward links may become unsync, a list might get tangled into a circular list, 
rendering some loops into infinite loops, etc. None of that can be avoided with `cobi` 
tools in themself. But, if we avoid STL iterators, we can at least know that there is no 
undefined behavior and all bugs should be reproducible and unit tests do give a high 
degree of assurance that code is correct. Also, one can see that checking for ranges of 
integers can lead to somewhat clumsy-looking code, but that's the price of safety.

### Checked links

A link of an element that was erased is harmless, but when a new element takes the slot that
one was in, the old link becomes the link of the new element. If that will not do, ask for
_checked_ links, by the name that has "checked" in it:

```cpp
cobcheckedlist<int, 64> l;
auto a = l.insert(l.lend(), 1);
l.erase(a);
auto b = l.insert(l.lend(), 2);     // in the slot that `a` was in
l.live(a);                          // false; in a `coblist`, `a` would be the link of 2
```

Then a link is the checked handle of the pool (see the pools): it also tells which of the
elements that were ever in the slot it is of. Everything that takes a link treats the link of
what is gone as the link of nothing, and so do the iterators. The cost is an `unsigned` per
element, a comparison whenever a link is used, and a link twice the size; a link is then not a
range bound integer, but it is still got from the container only and compared with `nil`.

There is one for each: `cobcheckedlist`, `cobcheckedfwdlist`, `cobcheckedskiplist`,
`cobcheckedmap`, `cobcheckedtree`, `cobcheckedtreemap`, `cobcheckedpool` and `cobcheckedhive`, with the template
arguments of the one that is not checked, and the same for the `atb` variants (`atbcheckedlist`
and so on). The timers have them too, see there.

### A bit for each slot

The pool keeps a byte for each slot to tell whether there is an element in it. `cobhive` is the
pool that keeps a bit instead (see the pools), and everything that is built on a pool can be
built on that one, by the name that has "hive" in it:

```cpp
cobhivelist<int, 4096> l;           // 512 bytes for that, where a coblist has 4096
```

It takes an eighth of the room for that, and a shift and a mask more each time an element is
got at by its link. Nothing else changes: the interface is the same, and so is what everything
does. There is one for each, and one that is checked as well: `cobhivelist` and
`cobcheckedhivelist`, and the same for `fwdlist`, `skiplist`, `map`, `tree`, `treemap`,
`tmrlist`, `tmrwheel` and `tmrmill`; then `cobhivelru` (the cache has no checked variant), and
all of them for `atb`. The lean timer mill, the lean timer ring and the containers that are not
built on a pool (vector, ring, hash table, flat set and map...) have none.

## Range bound  singly linked (forward) lists

Just like the ones from the STL, these save some memory and have slightly better performance
for most functions compared to their doubly linked counterpart, but can only be traversed one 
way (forward) and one can't simply (and efficiently) remove an item by position/iterator.

## Skiplists for associative containers

Skiplists are a good fit for this library, with each node being an array of links 
("pointers") to the next node per level. Also, this makes them easy to make cache-friendly, 
putting all  nodes in an array.

For code clarity, these are singly linked skiplists, which means that removing by iterator
is as efficient as removing by value. If some application requires better  performance for 
removing by iterator, a "doubly linked skiplist" should be implemented.

Parts of the STL associative containers interface that obviously "hint" that it expects they 
are implemented as red-black-trees are omitted. Other parts are missing simply because of a 
lack of need so far and are easilly added.

Skiplists use a random number generator to randomize the levels of each node. The default,
`cobhlp::xorshift32`, is small and deterministic, so the same insertions always build the same
structure. If the keys may be chosen by an adversary, pass your own generator type as the last
template parameter: `cobskiplist<int, 64, cobhlp::less<int>, MyRng>`. It must be default
constructible and callable as `rng()` returning an unsigned integer.

A skiplist keeps its size, so `size()` is O(1), has `full()`, and can be copied.

### Maps

`cobmap<K, V, N>` is a key-value view of the skiplist: what is kept is `{ key, value }`, ordered by
the key only (`cobmap<K, V, N, CMP, RNG>`, like for the skiplist). The key can't be changed, the
value can.

```cpp
cobmap<std::string, int, 64> m;
m.insert("two", 2);                         // { link, true }; { link of what is there, false } if the key is in
m.emplace("one", 1);                        // the value is made in place, from what follows the key
m.insert_or_assign("two", 22);
auto two = m.find<std::optional>("two");    // a copy of the value: std::optional<int>{22}
m.with("one", [](int& v) { ++v; });         // false if the key is not in
m.for_each([](std::string const& k, int& v) { /* in the order of the keys */ });
for (auto const& e : m) { use(e.key, e.value); }    // read only
m.erase("one");
```

`insert`, `emplace` and `insert_or_assign` return the link and whether a new element was put in; the
link is `nil` if there was no room. Like with the lists, a link is what to keep rather than an
iterator: `value<V>(link)`, `key<V>(link)`, `with(link, f)`, `erase(link)`, `live(link)`,
`where(key)`, `lower_bound(key)` and `upper_bound(key)` (which give links, to go on from with
`fwd(link)`). `find(key)` without the "optional" gives an iterator, like `std::map`.


## Optionals that take no extra room: `contingent`

`perhaps<T>` keeps a flag next to the value. `contingent<T>` does not: one value of what is
stored means "nothing", so `sizeof(contingent<T>)` is `sizeof` of what it holds. It has the same
interface as `perhaps` (`has_value`, `and_then`, `or_else`, `transform`, `value_or`, `reset`,
`==`), but, as there is no mutable access to the value, you assign a new `contingent`.

```cpp
contingent<int*>                      p;           // nullptr is nothing
contingent<double>                    d(1.5);      // NaN is nothing
contingent<int, nothing_is<-1>>       i(3);        // -1 is nothing
contingent<cobint<0, 5>>              c;           // 6 is nothing
auto r = coblist<int*, 3>{}.pop_front<contingently>();
```

The policy that says what "nothing" is, is `nothing<T>`, which you specialise for your own
types. Constructing a `contingent` from the sentinel itself gives an empty one. For
floating point numbers, NaN is kept as its bits and recognised by them, so that it works
with `-ffast-math` too (but do not compute with NaNs then, they are not _numbers_ any more).
`nulifier<T, nothing_is<-1>>` makes a `NULIFY` for `cobtmrleanmill` or `cobtmrleanring` out of a policy.

## Shared and unique pointers that are safe when null

`fullshared<T>` (shared ownership) and `fullunique<T>` (sole ownership) are never null, and
`optshared<T>` and `optunique<T>` may be null, but then there is no way to reach the object
without checking: no `*`, `->` or `get()`, only `and_then(f)`, which calls `f(T&)` if there is an
object. Create them with `make(args...)`; `fullshared<T>::from<std::optional>(shared_ptr)` checks a
pointer you have.

```cpp
auto a = fullshared<std::string>::make("abc");   // always valid
optshared<std::string> o = a;
o.and_then([](std::string& s) { s += "d"; });
```

A moved-from pointer would be null, so a `fullshared` is only ever copied (a "move" copies),
and a `fullunique` can be neither copied nor moved: it lives where it was made. An `optunique`
can be moved, and a moved-from one is simply null. `release()` gives the `unique_ptr` away.

## Allocation time bound containers: `atb`

The prefix `atb` is for containers whose dimension is not known at compile time, is fixed once
when the program starts, and whose access is nevertheless checked at compile time (like an Ada
subtype whose bounds are computed at elaboration). The dimension belongs to a _tag_ type and
can be set once. An index for that tag can only be made by a checked function, so an index that
exists is valid for every array of that tag, forever, and access needs no check.

```cpp
struct rows {};                                   // the tag
atbdim<rows>::set(read_the_size());               // once, before anything of the tag is used
atbarray<double, rows> a;
for (auto i : atbidx<rows>::range()) { a.set(i, 1.0); }
if (auto i = atbidx<rows>::be<std::optional>(n)) { a.set(*i, 2.0); }
```

If the dimension is used before it is set, it is 0 (and can not be set any more), and every
`be()` fails. There is no default constructor and no integer constructor for `atbidx`. An
`atbarray` is copied deep; there are no move operations, so a "move" copies and the source
stays usable. Everything that is a `cobarray`, including the iterators, is there.

Every other container has an `atb` variant too, in a header of the same name: `atblist<T, Tag>`,
`atbfwdlist<T, Tag>`, `atbskiplist<T, Tag>`, `atbmap<K, V, Tag>`, `atbpool<T, Tag>`,
`atbring<T, Tag>`, `atbbitset<Tag>`, `atbhash<K, V, Tag>`, `atbvector<T, Tag>`,
`atbbinheap<T, Tag>`, `atbstring<Tag>`, `atbmatrix<T, RowTag, ColumnTag>`, `atbtree<T, Tag>`,
`atbtreemap<K, V, Tag>`, `atbspsc<T, Tag>`, `atblru<K, V, Tag>`, `atbfunction<R(A...), Tag>`,
`atbflatset<T, Tag>`, `atbflatmap<K, V, Tag>` and `atbhive<T, Tag>`. The dimension of the
tag is the capacity (for a matrix, the number of rows and of columns). They are the very same
code as their `cob` namesakes, so the interface is the same, except that:

* Where a `cob` container takes an index that is checked at compile time (a bit of a bitset, an
  element of a vector or a matrix, a character of a string), the `atb` one takes an `atbidx<Tag>`.
  Links and handles (of lists, pools, maps) are types of their own, and `nil` compares with them.
* All the storage is on the heap, allocated once, when the container is made. Copies are deep
  and there are no move operations.
* Making a container - or a default constructed iterator of one, except of an `atbarray` - looks
  at the dimension, so it can not be set after that. With the dimension not set, the containers have no room at all, and
  everything fails as it does for a full container.
* An `atbstring` has room for at most 255 characters, even if the dimension is greater. As
  no string literal is known to fit at compile time, there is no constructor from one: use
  `assign()` or `from<V>()`, which tell if it did fit.
* An `atbskiplist` has as many levels as its dimension calls for.
* The dimension of an `atbfunction` (and of an `atbmovefunction`) is its room, in bytes. Nothing is known to fit at compile
  time, so it can't be made from what is to be called: `assign()` it.

The timers have them as well: `atbtmrlist<ID, Tag, U>`, `atbtmrwheel<ID, Tag, U, SPOKES>`,
`atbtmrmill<ID, Tag, U, LVL, DIM, EXACT>`, `atbtmrleanmill<ID, Tag, U, NULIFY, LVL, DIM>` and
`atbtmrleanring<ID, Tag, U, NULIFY>`. The
dimension of the tag is the number of timers, what `N` is for the `cob` ones; the numbers of
spokes and levels are still given at compile time. Each spoke gets its share of the timers the
same way: twice `N / SPOKES` for the wheel, `N / (LVL * DIM) + 1` for the mills - and none, if there
are no more timers than spokes, which is what the `cob` mills refuse to compile. The exact mill
has no shares: all the spokes use the same room. Nothing is allocated after a timer module is made.

Such a share is a dimension of its own, worked out from that of the tag: `atbpart<Tag, NUM, DEN,
ADD, MIN>` is a tag whose dimension is that of `Tag`, divided by `DEN`, multiplied by `NUM`, plus
`ADD` (or 0, if that of `Tag` is less than `MIN`). It can't be set, and it is there for whatever
you build of several `atb` containers yourself.

The dimension changes at most once, from "not set" to what it is from then on, and that is so
for all the threads: whichever of them looks at it, and whenever, they all see the same. So it is
fine to use `atb` containers of the same tag in many threads; it is only that the one that sets
the dimension has to do it before any other uses it, or it will find that it is too late.

### One implementation, two bounds

The containers are written once, as `bndlist<T, B>`, `bndvector<T, B>` and so on, where `B` is a
_bound_: a type that says how many slots there are and gives the types to index and to keep them
with. `cobbound<N>` (in `cobbound.hpp`) is the bound known at compile time: indices are range
bound integers and the storage is in the object. `atbbound<Tag>` (in `atbbound.hpp`) is the
bound set at run time. So, `coblist<T, N>` is `bndlist<T, cobbound<N>>` and `atblist<T, Tag>` is
`bndlist<T, atbbound<Tag>>`. The timers are done the same way (`bndtmrlist`, `bndtmrwheel`,
`bndtmrmill`, `bndtmrexactmill`, `bndtmrleanmill`, `bndtmrleanring`). You should not need the `bnd` names, but you will see them in
compiler messages. The arrays are `bndarray<T, B>`, in `bndarray.hpp`.

## Pools, ring buffers, bitsets and hash tables

No iterators here (each has `for_each`), and all of them hold any type of element, like the lists.

* `cobpool<T, N>`: an object pool. `make(args...)` gives a handle (`nil` if full), `drop(handle)`
  is `false` for a stale handle, `with(handle, f)`, `get(handle)`, `live(handle)`. Handles stay
  valid while other objects are made and dropped, also in a copy of the pool.
  The handle of a dropped object is harmless, but once a new object takes the slot, it is the
  handle of that one. If that will not do, `cobcheckedpool<T, N>` has _checked_ handles: each
  slot counts the objects dropped from it and a handle remembers the count it was made at, so
  the handle of a dropped object stays the handle of nothing. That takes an `unsigned` per slot
  and a comparison per access (and a handle is twice the size).
  `for_each` of a pool looks at every slot, however few objects there are. `cobhive<T, N>` is the
  pool that is good at going over its objects, like `std::hive` is: which slots hold an object
  is kept as a bit each, not a byte, so the ones that hold none are skipped 64 at a time, and
  `first()` and `next(handle)` go from object to object, in the order of the slots:
  `for (auto h = p.first(); h != p.nil; h = p.next(h))`. Asking if one slot holds an object
  costs a little more, which is why that is not what the pool always does. `cobcheckedhive<T, N>`
  has both that and the checked handles.
* `cobring<T, N>`: a first in, first out queue. `push_back`/`emplace_back` fail when full,
  `emplace_over` drops the oldest instead, `pop_front<V>()`, `front<V>()`, `back<V>()`, `at<V>(i)`.
* `cobbitset<N>`: bits indexed by `cobint<0, N-1>`, so `b.test(cobic<N>)` or `b.set(3)` do not
  compile; `maybe_set(int)` checks. All of it is `constexpr`. `first<V>()` and `next<V>(i)` walk the
  set bits.
* `cobhash<K, V, N, H, E>`: open addressing with linear probing, and deletion by backward shift, so
  there are no tombstones and it can be filled completely. `insert`, `insert_or_assign`, `erase`,
  `contains`, `find<V>(k)` (a copy), `with(k, f)`.

```cpp
cobhash<std::string, int, 64> h;
h.insert("one", 1);
auto one = h.find<std::optional>("one");   // std::optional<int>{1}
```

## Balanced search trees

`cobtree<T, N, CMP>` has the interface of the skiplist - a set of unique elements, ordered by
`CMP` - and one more guarantee: finding, inserting and erasing an element take O(log n) steps
_in the worst case_. A skiplist (like a hash table) is that fast on average, but there is no
bound on how slow a single operation may get, short of O(n). Where the time has to be bound,
use the tree.

It is an AVL tree. Of the balanced trees, that is the one of the least height - at most about
1.44 log2(n), where a red-black tree may get to 2 log2(n) - and each operation walks one path
from the root, so the least height is the tightest bound. (What a red-black tree is better at,
fewer rotations when erasing, does not change the bound.) Nothing in it is recursive, so the
stack it takes does not depend on the size.

The elements are kept in a `cobpool` and never move: a link or an iterator of an element is
good until that very element is erased. Iterators go both ways, and so do `fwd(link)` and
`bck(link)`; these, and `lbegin()`/`lback()`, are O(log n) too (and going over all the elements is
O(n)). `height()` tells how high the tree is. It keeps three links and a byte per element.

`cobtreemap<K, V, N, CMP>` is `cobmap` on the tree: the same interface, with the same guarantee.

```cpp
cobtree<int, 64> t;
t.insert(5);                        // { iterator, true }
for (int x : t) { use(x); }         // in order
cobtreemap<std::string, int, 64> m;
m.insert("one", 1);
```

## Flat sets and maps

`cobflatset<T, N, CMP>` is a set of unique elements kept in order in a `cobvector`, one next to
another, and `cobflatmap<K, V, N, CMP>` is the map of the kind. Compared to the tree and the
skiplist:

* There is nothing but the elements and their number: no links, no flags.
* Finding an element is a binary search over memory that is all in one place: O(log n) in the
  worst case, like the tree, and quicker than it.
* Inserting or erasing an element moves all those after it: O(n). `n` is at most `N`, so there
  still is a bound, and for a few dozen elements it is often less work than keeping a tree
  balanced. But filling a big one in no particular order is slow.
* Nothing stays where it was: an iterator or an index is good until the next insert or erase
  (after which it is harmless, as with a vector, but refers to another element, or none).
  There are no links and nothing to be "checked".
* The elements have to be such that they can be moved about.

So: for what is small, or is filled once and then mostly looked up. For what changes a lot,
is big, or where links must stay good, use `cobtree`.

The interface is that of the other sets and maps, with iterators where those have links:
`insert`, `emplace`, `erase`, `find`, `contains`, `count`, `lower_bound`, `upper_bound`,
`for_each`, and for the map `insert_or_assign`, `find<V>(key)` and `with(key, f)`. The iterators
are those of the vector (so, random access), read only. `index_of<V>(key)` gives the index of an
element as a "generic optional"; the index is known to be within the capacity, and is used with
`get(i)` / `at<V>(i)` (the set) or `key<V>(i)`, `value<V>(i)` and `with(i, f)` (the map).

```cpp
cobflatmap<std::string, int, 16> ports;
ports.insert("http", 80);
ports.insert("ssh", 22);
auto ssh = ports.find<std::optional>("ssh");            // std::optional<int>{22}
for (auto const& e : ports) { use(e.key, e.value); }    // in the order of the keys
```

## A queue between two threads

`cobspsc<T, N>` is a first in, first out queue for one producer and one consumer, each of which
may be a thread, or an interrupt handler, of its own. There are no locks and no waiting: each
operation takes a few steps whatever the other side is doing, and what is put in is taken out
once, in order. All `N` slots can be in use.

That there is only one of each is seen to by the queue: you put in through its `producer` and
take out through its `consumer`, and it gives each of them out once only -
`claim_producer<V>()` and `claim_consumer<V>()` give a "generic optional", with nothing in it
the second time. They can be moved, say to the thread that is to use them, but not copied; one
that was moved from does nothing. Like iterators, they are of use while the queue is there.

A side that is destroyed is not given back: the queue would not know that nobody else has it.
To hand a side over - say, to a thread that takes the place of one that is done -
`q.release(std::move(tx))` gives it back, after which it can be claimed again. What is in the
queue stays there.

The producer has `push(v)` and `emplace(args...)`, `false` if there is no room, and `full()`.
The consumer has `pop<V>()` (a "generic optional"), `with_front(f)`, `drop_front()` and `empty()`.
Anyone may ask the queue for `size()`, `empty()` and `full()`, which tell how it was a moment ago.
The queue can not be copied.

```cpp
cobspsc<sample, 64> q;
auto tx = q.claim_producer<std::optional>();    // for the interrupt handler
auto rx = q.claim_consumer<std::optional>();    // for the main loop
// in the interrupt handler:        if (!tx->push(s)) { ++dropped; }
// in the main loop:                while (auto s = rx->pop<std::optional>()) { process(*s); }
```

## Callables: `cobfunction`

`cobfunction<R(A...), BYTES>` holds anything that can be called with `A...` to give `R` - a
function, a lambda, an object - like `std::function` does, except that what it holds is kept
in the object itself, in `BYTES` bytes. So it never allocates, and what does not fit does not
compile (nor does what can't be called that way, or can't be copied). The default room is three
pointers: a function, a member function with its object, or a lambda that captures a few things.

Calling one that holds nothing is not an error: nothing is called, and `R{}` is returned if
there is something to return. If you need to know, `call(args...)` says: for `void` it returns
whether it called, otherwise `call<V>(args...)` gives the result as a "generic optional".
`assign(f)` is for what may not fit: it returns `false` and leaves the function as it was. A null
pointer to a function leaves it holding nothing.

`cobmovefunction<R(A...), BYTES>` is the same, but it can only be moved, not copied, and so it
can hold also what can only be moved (say, a lambda that captured a `std::unique_ptr`). What it
was moved from holds nothing. (A `cobfunction` that is "moved" is copied.)

```cpp
cobfunction<void(int)> on_expiry = [&log](int id) { log.note(id); };
on_expiry(5);
cobfunction<int(int), 8> f = [big_array](int) { return 0; };   // does not compile: does not fit
```

## A least-recently-used cache

`coblru<K, V, N, H, E>` keeps up to `N` keys with their values. When a new key is put in and
there is no room, the one that was not used for the longest time is dropped. `put(k, v)` puts or
changes the value, `get<V>(k)` gives a copy, `with(k, f)` lets you change it - these are a _use_
of the key. `peek<V>(k)`, `contains(k)` and `for_each(f)` (the most recently used first) are not.
`eldest<V>()` is the key that would go next, `erase(k)` takes one out. `put(k, v, gone)` calls
`gone(key, value)` with what is about to be dropped, so that it can be seen to (saved, closed).

It is a hash table and a list of use in the same `cobpool`. Each operation takes a few steps on
average; like in any hash table, if many keys hash alike, it takes as many steps as there are
such keys.

```cpp
coblru<std::string, page, 16> cache;
cache.put(name, load(name), [](std::string const& n, page& p) { save(n, p); });
auto p = cache.get<std::optional>(name);
```

## Vectors

`cobvector<T, N>` is an array that grows up to its capacity `N` and shrinks. Like in the lists,
elements are of any type, constructed when put in and destroyed when taken out. `push_back`,
`emplace_back`, `insert(pos, v)` and `emplace(pos, args...)` return `false` if there is no room;
`pop_back<V>()`, `front<V>()`, `back<V>()` and `at<V>(i)` give a "generic optional"; then
there are `erase(i)`, `get(i)`, `set(i, v)`, `with(i, f)`, `swap(i, j)`, `for_each(f)`, `resize`,
`clear`, `==` and the usual `size()`, `capacity()`, `empty()` and `full()`.

There are iterators, for the algorithms: `std::sort(v.begin(), v.end())` and the like, range-for,
`insert(it, v)`, `erase(it)` and `erase(first, last)`. They are like those of the array: they can't
be moved out of the vector (what would get out ends up at `end()`), and where there is no element -
at `end()`, or where there was one before the vector shrank - they refer to a "safety spot", so
dereferencing them is incorrect, but not undefined. An iterator that `insert` or `erase` could do
nothing with (not of this vector, no room) gives `end()`.

### How safe is the index

The index is a `cobint<0, N-1>`, so it is known _at compile time_ that no access is ever out of
the storage: `v.get(cobic<N>)` or `v.get(3)` do not compile. But whether there is an element at
an index depends on the size, which changes at run time, so that can not be known at compile
time. Access by index thus checks it (one comparison) and has a way to say "there is none":
`at<V>(i)` gives nothing, `set`, `with`, `erase` and `swap` return `false`, `get(i)` gives `T{}`.
An index kept from before the vector shrank is harmless, just like a stale link of a list.

What needs no check to fail, as the indices are those of the elements there are:

```cpp
cobvector<int, 8> v;
v.for_each([](int& x) { ++x; });
for (auto i : v.irange()) { use(v.get(i)); }     // the indices there are right now
```

We know of no way to make the access by index to a container of a changing size safe at compile
time in C++. These are the options and what they give:

1. What is done: the index is within the capacity (compile time), the element being there is
   checked (run time), and failing that check is not an error but a result. No undefined
   behavior and no error handling to forget.
2. Keep all `N` elements constructed, like `cobarray` does, and let the size be just a number.
   Then any index is fine at compile time, with no check at all, but one reads (or writes)
   elements "that are not there", and `T` must have a default constructor. That is how
   `cobstring` works, as characters are cheap to have. For a vector, that is a `cobarray` and a
   counter, which you can have without us.
3. A vector that only grows. An index that was valid stays valid, so a checked index could be
   a type that needs no further check - but only for one vector, which the type would have to
   name, like the tag of the `atb` containers does for a dimension. It can't shrink, though.
4. A view of a vector that is "frozen" while the view is there, with indices that belong to the
   view. C++ can't stop one from shrinking the vector while the view is there, so it would take
   a check at run time when shrinking, and the index type would still be shared by all the
   views made at the same place in the code.

## Binary heap

`cobbinheap<T, N, CMP>` is a priority queue kept in a `cobvector`. Like in `std::priority_queue`,
with the default "less" the greatest element is at the top; order by `std::greater<T>` to have
the smallest one there. `push`/`emplace` return `false` if there is no room, `pop<V>()` takes the
top out, `top<V>()` gives a copy of it and `with_top(f)` calls `f` with it. `for_each(f)` goes over
all the elements, in no particular order, without changing them.

```cpp
cobbinheap<int, 16, std::greater<int>> h;
h.push(3);
h.push(1);
auto least = h.pop<std::optional>();     // std::optional<int>{1}
```

## Strings

`cobstring<N>` holds at most `N` characters, and `N` is at most 255. It takes `N + 1` characters
and nothing else: the last one holds the number of characters that can still be added. So, with
a capacity of 5, it starts as 5, is 3 with two characters in, and 0 when the string is full -
which is then also the NUL that terminates it.

The interface is that of `std::string`, less what is unsafe:

* There are no iterators and no references or pointers to characters (no `operator[]`, `at()`,
  `front()`, `back()`, `data()` or `c_str()`). `get(i)` takes a `cobint<0, N-1>`, so it is within the
  capacity at compile time; past the last character it gives NUL. `set(i, c)` returns `false` if
  there is no character at `i`. `front<V>()`, `back<V>()` and `pop_back<V>()` give a "generic
  optional".
* What changes the string returns `bool`: `false` means the result would not fit, or the position
  is past the end, and then the string is as it was. Nothing throws and nothing is cut short:
  `assign`, `append`, `push_back`, `+=`, `insert`, `erase`, `replace`, `resize`.
* A string literal is checked to fit at compile time, as is a `cobstring` of a smaller (or the
  same) capacity. What may not fit goes through `assign()` or `from<V>()`.
* Searching gives the index of the character where it was found, or nothing, rather than a
  number that may be `npos`: `find<V>`, `rfind<V>`, `find_first_of<V>`, `find_last_of<V>`,
  `find_first_not_of<V>`, `find_last_not_of<V>`. The result is an index, so it needs no checking
  to be used (which also means that an empty string is not found at the very end).
* The rest is as usual: `size`, `length`, `capacity`, `empty`, `full`, `clear`, `substr`,
  `contains`, `starts_with`, `ends_with`, `compare`, `==`, `<`, and the like (with any
  `cobstring`, `std::string`, `std::string_view` or literal), `swap`, `<<` to a stream and
  `std::hash`. Arguments are `std::string_view`, and may be a part of the string itself.
* `view()` (and the conversion to `std::string_view`) is how the string is passed on. A view is
  commonly understood to be a parameter, not something to keep: it refers to the characters of
  the string, so it is of no use once the string is gone. For the same reason there is no
  `c_str()` - a pointer to characters is what people do keep.

All of it is `constexpr`.

```cpp
cobstring<15> s = "hello";                  // cobstring<4> s = "hello"; does not compile
if (!s.append(", world")) { /* did not fit, s is still "hello" */ }
if (auto at = s.find<std::optional>(',')) { s.set(*at, ';'); }
std::cout << s << '\n';           // or anything that takes a std::string_view
```

## Timers

There are several timer "modules" available. Their interface is almost
the same, all support starting, stopping by ID and processing the
expired ones; most also support stopping by index (which you get on
starting). The lean ring has no index, as inserting moves timers
around (see [timers.md](timers.md)).

How to choose which one to use:

1. If you have a list of at most a several dozen timers, just use the
   "simple" timer list. It's very flexible and works predictibly
   well, with no precision loss and such.
2. For up to a few hundred timers, use the wheel to get better
   performance, reducing the list of each spoke to several dozen
   timers. The number of spokes should not be large, in general, 10
   is a good choice.
3. If you are OK with progressive loss of precision, regular mill
   is the best. If you need a smaller footprint and have a way to
   distinguish a cancelled timer, use the lean mill. If you want the
   mill, but can't have the loss of precision, use the exact mill
   (see below).
4. If memory is tight and timers mostly expire (are rarely cancelled),
   use the lean ring. It keeps the timers sorted by expiry in a ring
   with no links, no pool and no per-timer bookkeeping, so it has the
   smallest footprint of them all. There is no stop by index;
   cancelling nullifies a timer in place and a cancelled timer is
   never reported. Starting is O(N), so it suits modest numbers of
   timers.
5. Otherwise, use a "high precision" timer list, which is simply an
   associative container where each element holds the time at which
   timer should expire. This is currently not implemented, because
   it's pretty simple to do, just use the skip list as the container.

Keep in mind that these do not enforce the uniquiness of timer IDs in
no way, it's up to the user to do so. That is, if you start two or
more timers with the same ID and then stop by that same ID, there
are no guarantees which of the two will be stopped.

Also, there is no "garbage collection" here. If a timer expires and
then you stop it by index, nothing happens, unless a new timer was
started in the meantime and got the same index. In that case you will
stop that new timer.

If you push timer expiry to some event queue, then there is a chance
user might try to stop said timer while in queue. There's no support
for handling this situation. You need some bookeeping on your side.

See [Timer Interface Spec](timers.md).

### The exact mill

The last template parameter of the mill, `EXACT`, which is `false` unless you say otherwise,
makes it exact:

```cpp
cobtmrmill<int, 512, std::chrono::milliseconds, 3, 64, true> mill;
```

In the regular mill a timer stays in the spoke it was put in when started, and the spokes of the
further wheels are a whole turn of the previous wheel apart. In the exact mill, the timers are
_cascaded_: each time a wheel has turned around, the timers of the spoke that is due at the next
wheel are put anew, by what is left of their time. So each timer gets to the first wheel in time
and expires precisely: on the tick after its duration, as the first level of the regular mill does.

The price: each timer also keeps the tick it is due at, a timer of level `L` is moved up to `L`
times before it expires, and the tick on which a wheel turns around does that moving. On the
other hand, all the spokes share the room for timers, so there can be `N` running timers
whatever their durations (in the regular mill, a spoke has a room of its own, and a timer can't
be started if its spoke is full). The interface, the maximal duration and the cost of starting,
O(1), and of stopping by index, O(1), are the same; `stop_first()` looks at all the timers.

The index that `start()` gives is checked: once the timer has expired or was stopped, it is the
index of nothing, also when a new timer takes its place - so stopping by an old index never
stops some other timer.

### Checked indices

In the other timer modules, the index of a timer that is not running any more may get to be
the index of another timer, started since: stopping by an index that is that old is up to you
not to do. If that will not do, there is a variant of each with _checked_ indices, which stay
the index of nothing: `cobcheckedtmrlist`, `cobcheckedtmrwheel`, `cobcheckedtmrmill` and
`cobcheckedtmrleanmill` (and `atbcheckedtmrlist` and so on), with the same template arguments.
For the list, the wheel and the mill, that takes what checked links of a list take: an
`unsigned` per timer. For the lean mill it is an `unsigned` per spoke. An index gets bigger.


### Using timers as a deadline scheduler

Event to be processed by a deadline can be presented by a timer that
will expire on that deadline. With a way to _force_ the timer at the 
head of the list to expire, you have the equivalent of dequeuing the
next event to process from the event queue.

If a timer _really_ expires, that means its deadline was not met.

What we provide is "stop the first timer" which will return the ID of
that timer. Use that to dequeue the next event to process, and you'll 
have a deadline scheduler.
