# What all the containers share

This is what the other documents refer to, so that it is said once.
Each class, or group of classes, has a document of its own:

| Document                           | Classes                                                     |
|------------------------------------|-------------------------------------------------------------|
| [cobi.md](cobi.md)                 | `cobi`, `cobint`, `cobic`, `cobirange`                      |
| [atbdim.md](atbdim.md)             | `atbdim`, `atbidx`, `atbpart`, and what `atb` variants are  |
| [cobarray.md](cobarray.md)         | `cobarray`, `atbarray`                                      |
| [cobmatrix.md](cobmatrix.md)       | `cobmatrix`, `atbmatrix`                                    |
| [cobvector.md](cobvector.md)       | `cobvector`, `atbvector`                                    |
| [cobstring.md](cobstring.md)       | `cobstring`, `atbstring`                                    |
| [cobbitset.md](cobbitset.md)       | `cobbitset`, `atbbitset`                                    |
| [cobpool.md](cobpool.md)           | `cobpool`, `cobcheckedpool`, `cobhive`, `cobcheckedhive`    |
| [coblist.md](coblist.md)           | `coblist`, `cobfwdlist`                                     |
| [cobskiplist.md](cobskiplist.md)   | `cobskiplist`, `cobtree`                                    |
| [cobmap.md](cobmap.md)             | `cobmap`, `cobtreemap`                                      |
| [cobflat.md](cobflat.md)           | `cobflatset`, `cobflatmap`                                  |
| [cobhash.md](cobhash.md)           | `cobhash`                                                   |
| [coblru.md](coblru.md)             | `coblru`                                                    |
| [cobbinheap.md](cobbinheap.md)     | `cobbinheap`                                                |
| [cobring.md](cobring.md)           | `cobring`                                                   |
| [cobspsc.md](cobspsc.md)           | `cobspsc`                                                   |
| [cobfunction.md](cobfunction.md)   | `cobfunction`, `cobmovefunction`                            |
| [perhaps.md](perhaps.md)           | `perhaps`, `contingent`                                     |
| [fullptr.md](fullptr.md)           | `fullptr`, `optptr`, `fullshared`, `optshared`, `fullunique`, `optunique` |
| [timers.md](timers.md)             | `cobtmrlist`, `cobtmrwheel`, `cobtmrmill`, `cobtmrleanmill`, `cobtmrleanring` |

Where a class mimics one of the Standard library, its document only
says how it differs from that one. Where there is no Standard
counterpart, the document describes all of it.


# The names

A name tells what kind of a bound the container has, and which
options it was made with:

```
cob        list        the capacity is given at compile time
atb        list        the capacity is set at run time, once
cob checked list       links are checked
cob hive    list       which slots are in use is kept as a bit each
cob checkedhive list   both
```

So there are `coblist`, `atblist`, `cobcheckedlist`, `atbcheckedlist`,
`cobhivelist`, `atbhivelist`, `cobcheckedhivelist` and
`atbcheckedhivelist`. All of them are the same class template,
`bndlist`, with different arguments; you will see the `bnd` names in
compiler messages, but should not need to write them.

Not every class has every option: `checked` and `hive` are only there
for what keeps its elements in a pool. Each document lists the names
there are.

An `atb` name takes a tag type where the `cob` name takes the number:
`coblist<T, 64>`, `atblist<T, Tag>`. See [atbdim.md](atbdim.md).


# The capacity is fixed

```cpp
size_type size() const;
size_type max_size() const;
bool      empty() const;
bool      full() const;
```

No container ever allocates after it was made, and none can hold more
than its capacity, which is what `max_size()` gives. So whatever puts
an element in can fail for the lack of room, and says so by what it
returns: `false`, `nil`, the `end()` iterator or an empty optional,
depending on the function. Nothing throws, and nothing is cut short:
if all of what was asked for does not fit, nothing is done.

`full()` tells whether the next element would fit. It is not in the
Standard containers.

`size()` is O(1) for all of them.


# Elements of any type

Unless its document says otherwise, a container holds elements of any
type: an element is constructed when it is put in and destroyed when
it is taken out, and only as many objects exist as there are elements.
The type need not have a default constructor, nor be copyable.

What does need something of the type says so:

* `get()` returns a copy, or `T{}` when there is no element, so it
  needs both a copy and a default constructor.
* Dereferencing an iterator that refers to no element gives a
  "safety spot" (see below), which is a `T{}`.
* Copying a container copies the elements.

The containers assume that constructing, copying and moving an element
does not throw.

Containers can be copied, and a copy is deep. Those that construct
and destroy their elements have no move operations: a "move" copies,
and what was "moved from" stays as it was. (`cobspsc` can be neither
copied nor moved.)


# The generic optional: `V`

```cpp
template <template <class> class V> V<T> front() const;

auto x = list.front<std::optional>();
```

Where a Standard container returns a reference to an element that
might not be there (and leaves it to you not to ask when it is not),
these containers return a copy in an optional. Which optional is up
to you: it is the template argument `V`, and all that is asked of it
is

* `V<T>{}` is "nothing", and
* `V<T>(value)` is "this value".

`std::optional` does, and so do `perhaps` and `contingently` of this
library (see [perhaps.md](perhaps.md)). If you only want the effect
and not the value, a type that ignores what it is given does too:

```cpp
template <class U> struct ignore {
    ignore() {}
    ignore(U const&) {}
};
list.pop_front<ignore>();
```

In a template, the call has to be spelled `c.template front<V>()`.


# `with()` and `for_each()`

```cpp
template <class F> bool with(link l, F&& f);
template <class F> void for_each(F&& f);
```

A copy is not always what is wanted, and not every type can be
copied. `with()` calls `f` with a reference to the element, if there
is one, and returns whether it did. `for_each()` calls `f` with each
element, in the order of the container.

The reference is good only during the call; do not keep it. `f` must
not change the container it was called from.

When the container is `const`, `f` gets a reference to `const`.


# Links

```cpp
link lbegin() const;
link lend() const;          // nil
link fwd(link l) const;
bool live(link l) const;
static constexpr auto nil;
```

The containers that keep their elements in a pool (lists, skiplists,
trees, maps) give each element a `link`: a small value that tells
where the element is. It is what an iterator is in the Standard
containers, except that a link is good on its own - it does not point
into the container, so it can't dangle.

* A link is got from the container only, and compares with `nil`.
* An element stays at its link for as long as it is in the container,
  whatever is done with the other elements.
* `live(l)` tells whether there is an element at the link, in O(1).
* What takes a link does nothing if there is no element at it, and
  says so by what it returns. (Going on from such a link, with
  `fwd()`, is safe too, but where it gets you is specified only for
  the `checked` variants: there it is `nil`.)

The link of an element that was erased is harmless. But once a new
element is put in the slot that one was in, the old link is the link
of the new element. If that will not do, use the `checked` variant.

For a `cob` container the link is a range bound integer,
`cobint<0, N>`, with `N` being `nil`. Do not make one yourself.


# Checked links

```cpp
cobcheckedlist<int, 64> l;
auto a = l.insert(l.lend(), 1);
l.erase(a);
auto b = l.insert(l.lend(), 2);     // in the slot that `a` was in
l.live(a);                          // false
```

In a `checked` variant a link also tells which of the elements that
were ever in its slot it is the link of. So the link of an element
that is gone stays the link of nothing, and so does an iterator that
was at it.

It costs an `unsigned` for each slot, a comparison for each use of a
link, and the link is about twice the size. Such a link is a class of
its own: it can be default constructed (to `nil`), copied and compared
for equality, and that is all.

The counter is an `unsigned`. Only after a slot was reused as many
times as that can count could a link that old be taken for a new one.


# Hive: a bit for each slot

```cpp
cobhivelist<int, 4096> l;
```

The pool keeps, for each slot, whether there is an element in it. By
default that is a byte for each slot. In a `hive` variant it is a bit:
that takes an eighth of the room, and costs a shift and a mask more
whenever a link is used.

Nothing else changes. The interface is the same, and so is what every
function does and returns.

The name is that of the pool with this option, `cobhive`, which can
also go from element to element (see [cobpool.md](cobpool.md)). The
containers built on it do not offer that: they have an order of their
own.


# Iterators

Iterators are there so that range-for and the Standard algorithms can
be used. For anything else, links and indices are the better choice.

They differ from the iterators of the Standard containers in what
happens when they are misused:

* An iterator that refers to no element - `end()`, one whose element
  was erased, one moved past the end - can be dereferenced. It gives
  a reference to a "safety spot", an object that is there for the
  purpose (one for each thread). What you read is of no use, and what
  you write is lost, but the behaviour is defined.
* An iterator can't be moved out of its container: what would get out
  stays at `end()`. (Where an iterator whose element was erased gets
  when it is moved is specified only for the `checked` variants:
  `end()`. Otherwise it is safe, but may be anywhere.)
* A default constructed iterator is the `end()` of a container that
  is empty (one for each thread), so it is safe to use too.
* `contains(it)`, where there is one, tells whether the iterator is of
  this container and at an element.

What is left undefined is using an iterator after its container is
gone. That can't be helped in an iterator.

The iterators satisfy the C++20 iterator concepts of the category
that each document states.
