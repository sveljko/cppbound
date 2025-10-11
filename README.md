# Compile time bound, and thus safe, types

For description in Serbian, see: [Опис на српском](README.sr.md).

This is a collection of types that are statically - at compile time - bound
so that you can't accidentally misuse them and get undefined behavior. They are,
thus safe. This is C++, you can still cast away all this safety or do other nasty
things, but, there's nothing like "what if my integer addition overflows" or
"what if I call front() on an empty std::vector". Also, there's no dreaded
"should I throw or return an error if I detect invalid operation at runtime".

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
* Dimension bound matrices - two dimensional arrays. Needed to avoid copying
  whole rows on accessing elements.
* Pointers that are never null(ptr). We call them `fullptr<T>`. While there are
  similar implementations that check that pointer is not `nullptr` at runtime,
  here we prevent constructing a `fullptr<>` from a pointer that is `nullptr`. 
  While that prevention is done at runtime, the `fullptr<>` is known at compile
  time to _never_ be null. They have no notion of ownership and you can delete
  the "underlying" pointer if you wish.
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
* Capacity bound skip lists. Currently only model the `std::set<>` interface.
  If the need arises, should be simple to add `std::multiset<>`, `std::map<>`
  and `std::multimap<>` support. Other remarks are similar to general capacity 
  bound lists above.
* Capacity bound Timer lists. Very simple and flexible. One can start
  a timer - O(N), stop it by ID - O(N), stop by index gotten when
  starting - O(1) and process the ones that have expired - O(1).
* Capacity bound Timer list wheel. An optimization with a number of
  "spokes" each being a timer list and used for timers that expire
  "modulo spokes". Thus inserting and searching is "spokes times
  faster". It loses some flexibility as it now has to be based on
  "ticks", the durations cannot be arbitrary any more. Also, there is
  a loss in precision proportional to the number of spokes. One can
  get around this loss of precision by trading it off with loss of
  performance, if on each "tick" each spoke is checked for expired
  timers (rather than only the "next" spoke being checked).
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

This should provide, with the caveats mentioned above, the same level
of safety as, say, Ada/Spark's "no runtime errors". It's actually
higher safety than most other languages, including various "safe"
ones, such as "plain" Ada, that catch such errors at runtime.

Other fundamental types such as strings, vectors, hash tables and
search trees are not implemented at this time. They might be added in the future.

## Getting started

Just copy the headers you need where you'd like them to be and include them.

This is designed for and tested with C++17. It should work with few changes in C++14, let us
know if you need help with that.

## Usage

Should be rather simple. Look at the [examples](bound.t.cpp). Read the docs if you need more
info. Since this is a header-only library, you only need to set the include directories.

If you're using the `meson` build tool, there's a `meson.build` so that you can use this as
a meson subproject, which should take care of include directories.

## Basic design decisions

Using templates and constant expressions, check for bounds at compile time.

To the extent possible, make it so that the generated code is essentially the
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
    usbi z = x + y; //!! final range is 0,6, upper bound higher than usbi's 5
```

and this is just asking for it;

```cpp
    usbi x{};
    usbi y{};
    usbi z = x + y; //!! final range is 0,10!
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


### Compiler error messages

TLDR; they're not great.

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

For those, we provide a shortcut:

```cpp
    cobint<0,5>
```

is the same as

```cpp
    cobi<int, 0,5>
```

This is needed because we can't have `int` be a default type and
define a range (at best we could have the default range of `0,0` for the default `int` type). 
For constants, there's `cobic`, which can save a few strokes  and look nicer in expressions:

```cpp
    cobint<0,5> x;
    auto r = x + cobint<8>{};
    auto q = x + cobic<8>;
```


## Range bound arrays

Other than using Range Bound Integers for the index, thus knowing at compile time that there's no
out of array bounds access, we also don't follow the STL interface "to the letter".

We don't give the reference to any member - you get copies. While this may be a performance
penalty it also means that there's no means to abuse the provided reference. This will
*not* compile:

```cpp
    cobarray<int, 3> a;
    int& r = a.get(cobic<0>); // cannot bind temp to non-const reference
```

While you can find your way into Undefined Behavior with, say, `std::array<>`:

```cpp
    int& f() {
        std::array<int, 3> a;
        return a[1];
    }
    int& r = f();
    r = 5; // !!! writing to a dangling reference
```

Sure, for such obvious cases your compiler might produce a warning, but for real-life
problems, you probably won't be so lucky. Also, errors are better than warnings.

Another interesting thing is that we allocate one extra element as the "landing zone" for the
"iterator out of bounds". So, if you push the iterator out of the bounds of the array, it will
"sit" in the landing zone and dereferencing it will give you "something". It's not undefined,
it's actually well defined in any particular program, we just don't care what it is (you could
say it's "unspecified"). Unfortunately, this safety comes with an additional price of extra
checks in the iterator.

If you need the lost speed _and_ if you're only gonna use the iterators with standard algorithms 
and the `for` ranged loop, then it should be fine to remove the helper iterator class and just 
use the pointers as iterators, as these should never "do the nasty".

A safe way to iterage through the array is to use a helper range iterator, like:

```cpp
    cobarray<WHATEV, DIM> a;
    for (auto i: a.irange()) {
        a.set(i, SOMETHING);
        std::cout << a.get(i);
    }
```

One thing, though - we don't initialize the members of the array, because arrays can be very
big and that might be detrimental to the performance. Of course, the default constructor will
be called for each element, so be mindful of what you put into this array.

### But I have a large struct to keep in the array!

Obviously, if the struct is very large, then copying it around all the time just to get/set 
a small member implies low performance. To circumvent that, there are overloaded `get()` 
and `set()` to be used to get set a data member. You just need to pass the pointer-to-member, 
too. Something like:

```cpp
    struct S {
        int i[200];
        float f;
    };
    cobarray<S, 3> as;
    as.set(cobic<0>, &S::f, 2.0f);
    assert(2.0 == as.get(cobic<0>, &S::f));
```


## Range bound lists

Lists maintain a helper stack of free elements. The next (and previous)
"pointers" are actually indeces to said elements, and are kept in helper arrays of indeces.

Like for arrays, we have one more element than "asked for" and the "end()" iterator points 
to it. We use its index as the equivivalent of a `nullptr` from the usual "dynamically
allocated lists".

The interface of `std::list` is mostly there and behaves similarly, but those functions that
use iterators should not be used unless you have to. Use the alternate/mimicked functions
which accept the indeces, which are safe. For sanity checks, you can use `contains()` which
will check whether the given index is in the list. It's slow (O(N)), but, provides a higher
level of ensurance of correctness.

If you take a look at the code, you will notice that there may be many problems in the code
which cannot be addressed with our compile-time bound integers and arrays. The forward
and backward links may become unsync, a list might get tangled into a circular list, 
rendering some loops into infinite loops, etc. None of that can be avoided with `cobi` 
tools in themself. But, if we avoid STL iterators, we can at least know that there is no 
undefined behavior and all bugs should be reproducible and unit tests do give a high 
degree of assurance that code is correct. Also, one can see that checking for ranges of 
integers can lead to somewhat clumsy-looking code, but that's the price of safety.

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

Skiplists rely on a good RNG to randomize the levels of each node, thus we currently 
"hard-code" to a good, well-known RNG from the C++ standard library. This requires 
further study on how to allow the user to parameterize RNG w/out performance degradation.


## Timers

There are several timer "modules" available. Their interface is almost
the same, all support starting, stopping by ID, stopping by index
(which you get on starting) and processing the expired ones.

How to choose which one to use:

1. If you have a list of at most a several dozen timers, just use the
   "simple" timer list. It's very flexible and works predictibly
   well, with no precision loss and such.
2. For up to a few hundred timers, use the wheel to get better
   performance, reducing the list of each spoke to several dozen
   timers. Choose the quick variant if you can accept the loss of
   precision. The number of spokes should not be large, in general, 10
   is a good choice.
3. If you are OK with progressive loss of precision, regular mill
   is the best. If you need a smaller footprint and have a way to
   distinguish a cancelled timer, use the lean mill.
4. Otherwise, use a "high precision" timer list, which is simply an
   associative container where each element holds the time at which
   timer should expire. This is currently not implemented, because
   it's pretty simple to do, just use the skip list as the container.

Keep in mind that these do not enforce the uniquiness of timer IDs in
no way, it's up to the user to do so. That is, if you start two or
more timers with the same ID and then stop by that same ID, there
are no guarantees which of the two will be stopped.

Also, there is no "garbage collection" here. If a timer expires and
then you stop it by index, it will not be ignored.  If there is a new
timer started with the same index, you will stop that new timer rather
than "your own". If no timer is started currently, there's a chance
you'll get a corrupted list. So, do _not_ stop by index unless you're
sure the timer is still running.

If you push a timer expiry to some event queue, then there is a chance
user might try to stop a timer after it's removed from the timer
module, but still in the queue, not processed yet. There is no support
to handle this situation, if stopping such timer is needed, user
should have some additional map of "expired but not yet handled
timers".

See [Timer Interface Spec](timers.md).


### Using timers as a deadline scheduler

Event to be processed by a deadline can be presented by a timer that
will expire on that deadline. The only thing that is needed is a way
to "get the first timer to expire and cancel it", which is equivalent
of dequeuing the next event to process from the event queue.

If a timer expires, that means its deadline was not met.

So, we provide an unusual "stop the first timer" which will return the
ID of that timer, if it exists. Use that to dequeue the next event to
process, and you'll have a deadline scheduler.

