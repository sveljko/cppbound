# Lists

```cpp
#include "coblist.hpp"      // or "atblist.hpp"

coblist<T, N>               atblist<T, Tag>
cobcheckedlist<T, N>        atbcheckedlist<T, Tag>
cobhivelist<T, N>           atbhivelist<T, Tag>
cobcheckedhivelist<T, N>    atbcheckedhivelist<T, Tag>

#include "cobfwdlist.hpp"   // or "atbfwdlist.hpp"

cobfwdlist<T, N>            atbfwdlist<T, Tag>
cobcheckedfwdlist<T, N>     atbcheckedfwdlist<T, Tag>
cobhivefwdlist<T, N>        atbhivefwdlist<T, Tag>
cobcheckedhivefwdlist<T, N> atbcheckedhivefwdlist<T, Tag>
```

`coblist` is a doubly linked list of at most `N` elements, with the
interface of `std::list`. `cobfwdlist` is the singly linked one, with
the interface of `std::forward_list`. This document says how they
differ from those two.

The elements are kept in a pool ([cobpool.md](cobpool.md)) that is a
part of the list, and the "pointers" from element to element are the
numbers of their slots. Read [common.md](common.md) first: what it
says about capacity, the generic optional `V`, links, checked links,
the hive option and iterators all goes for the lists.


# What is not there

Of `std::list` and `std::forward_list`, these are missing:

* Allocators, and the constructors other than the default and the
  copy constructor. There is no construction or assignment from a
  count, a range or an initializer list, and no `assign()`.
* `resize()`.
* Reverse iterators (`rbegin()` and the like).
* `insert()` of a count, a range or an initializer list.
* `erase()` of a range.
* `splice()` of a single element or of a range; for `cobfwdlist`,
  all of `splice_after()` and `before_begin()`.
* Comparison of lists (`==`, `<=>`).
* The non-member `swap()`, `erase()` and `erase_if()`.
* Move construction and assignment (a "move" copies).


# What is different


## Putting in can fail

```cpp
template <class... A> bool emplace_front(A&&... a);
template <class... A> bool emplace_back(A&&... a);      // coblist
bool push_front(T const& t);
bool push_front(T&& t);
bool push_back(T const& t);                             // coblist
bool push_back(T&& t);                                  // coblist
```

These return `bool` rather than nothing (or a reference): `false` if
there is no room, in which case nothing is done.


## Taking out gives the element

```cpp
template <template <class> class V> V<T> pop_front();
template <template <class> class V> V<T> pop_back();    // coblist
```

Return the element that was taken out, moved into a generic optional,
or nothing if the list is empty. In the Standard these return nothing
and are undefined for an empty list.


## The first and the last element are copies

```cpp
template <template <class> class V> V<T> front() const;
template <template <class> class V> V<T> back() const;  // coblist
```

Return a copy in a generic optional, not a reference; nothing if the
list is empty. To change the element where it is, use `with()` or
`set()` with `lfront()` (or `lback()`).


## Inserting by an iterator

```cpp
iterator insert(const_iterator pos, T const& value);    // coblist
iterator insert(const_iterator pos, T&& value);         // coblist
iterator insert_after(const_iterator pos, T const& value);   // cobfwdlist
iterator insert_after(const_iterator pos, T&& value);        // cobfwdlist
```

As in the Standard, but if the element could not be put in, `end()`
is returned. That is so when there is no room and when `pos` is at no
element - except that `insert()` at `end()` puts the element at the
end of the list, as usual. `insert_after()` at `end()` puts nothing
in.

There is no `emplace()` or `emplace_after()` that takes an iterator:
those take a link, see below.

Only the link of the iterator is looked at. If you pass an iterator of
another list of the same type, it is taken for whatever of this list
is at the same link.


## Erasing by an iterator

```cpp
iterator erase(const_iterator pos);                 // coblist
iterator erase_after(const_iterator pos);           // cobfwdlist
```

As in the Standard, except that it is not an error if there is nothing
to erase: nothing is done, and `end()` is returned. As with inserting,
only the link of the iterator is looked at.


## All of another list

```cpp
bool splice(link pos, bndlist& x);                  // coblist
bool merge(bndlist& x);
template <class C> bool merge(bndlist& x, C comp);
void swap(bndlist& x);
```

`splice()` takes a link, not an iterator: the elements of `x` are
moved before the element at `pos`, or to the end if `pos` is `nil`.

`splice()` and `merge()` return `false`, and change nothing, if the
elements of both lists would not fit in this one, if `x` is this list
and, for `splice()`, if `pos` is neither `nil` nor the link of an
element.

Unlike in the Standard, the elements do not stay where they are: each
is move constructed into a slot of this list. So `T` has to be
movable, and links and iterators to the elements of `x` are not links
to them in this list - `x` is left empty, and they are links of
nothing.

`swap()` copies both lists, so it is O(n), needs `T` to be copyable,
and the links of each list refer, after it, to whatever is in the
same slot.


## Sorting

```cpp
void sort();
template <class C> void sort(C comp);
```

As in the Standard: stable, O(n log n), and only the links between
the elements change, so links and iterators stay at their elements.
It is a merge sort that recurses to the depth of log2(n).

`remove()`, `remove_if()` and `unique()` return the number of
elements removed (as they do since C++20), and `reverse()` is as in
the Standard.


## Iterators

Bidirectional for `coblist`, forward for `cobfwdlist`. They behave as
[common.md](common.md) says. In particular:

* `--` of `end()` gives the last element, as usual; `--` of the first
  element gives `end()`.
* An iterator whose element was erased refers to the safety spot.
  In a checked list, `++` and `--` take it to `end()`. Otherwise,
  where they take it is not specified (it is safe to use, but may be
  at an element or not), and the iterator is at an element again when
  one is put in the same slot.

```cpp
bool contains(const_iterator pos) const;
```

Tells whether the iterator is of this very list and at an element.


# What is added

The interface by links. Everything here is safe to call with any
link, at any time: with the link of no element, nothing is done.


## Getting the links

```cpp
link lbegin() const;
link lfront() const;
link lback() const;         // coblist
link lend() const;
link fwd(link l) const;
link bck(link l) const;     // coblist
```

`lbegin()` and `lfront()` are both the link of the first element,
`lback()` that of the last one; `nil` if the list is empty. `lend()`
is `nil`.

`fwd(l)` is the link of the element after the one at `l`, and
`bck(l)` of the one before it; `nil` if there is no such element.

If there is no element at `l`, a checked list gives `nil`. A list
that is not checked gives a link that is not specified: `nil`, or the
link of some slot, with an element in it or not. It is safe to use
like any other link, but it is of no use; if a link might be that of
an erased element, ask `live()` before going on from it.

```cpp
for (auto l = list.lbegin(); l != list.lend(); l = list.fwd(l)) {
    use(list.get(l));
}
```


## Is there an element

```cpp
bool live(link l) const;
bool contains(link l) const;
```

Both tell whether there is an element at the link. O(1).


## Access by a link

```cpp
T get(link l) const;
bool set(link l, T const& t);
bool set(link l, T&& t);
template <class F> bool with(link l, F&& f);
template <class F> bool with(link l, F&& f) const;
```

`get()` returns a copy of the element, or `T{}` if there is none.

`set()` assigns to the element. Returns `false` if there is none.

`with()` calls `f` with a reference to the element and returns `true`;
if there is no element, `f` is not called and `false` is returned.


## Insert by a link

```cpp
template <class... A> link emplace(link pos, A&&... a);         // coblist
link insert(link pos, T const& value);                          // coblist
link insert(link pos, T&& value);                               // coblist

template <class... A> link emplace_after(link pos, A&&... a);   // cobfwdlist
link insert_after(link pos, T const& value);                    // cobfwdlist
link insert_after(link pos, T&& value);                         // cobfwdlist
```

For `coblist`, the new element is put before the one at `pos`, or at
the end if `pos` is `nil`. For `cobfwdlist`, it is put after the one
at `pos`.

Returns the link of the new element. Returns `nil`, and puts nothing
in, if there is no room or if `pos` is the link of no element (and,
for `coblist`, is not `nil`).


## Erase by a link

```cpp
link erase(link pos);               // coblist
link erase_after(link pos);         // cobfwdlist
```

`erase()` takes out the element at `pos` and returns the link of the
one that followed it. `erase_after()` takes out the element after the
one at `pos` and returns the link of the one that followed that.

Both return `nil` if there is no such following element, and also if
nothing was erased, because there was nothing at `pos` (or after it).


## Go over all the elements

```cpp
template <class F> void for_each(F&& f);
template <class F> void for_each(F&& f) const;
```

Calls `f` with a reference to each element, first to last.


## Is it full

```cpp
bool full() const;
```

Whether there is no room for another element.


# What it takes

For each of the `N` slots: room for a `T`, a link to the next element,
for `coblist` a link to the previous one, a byte (or a bit, in the
hive variants) that tells whether the slot is in use and, in the
checked variants, an `unsigned`. A link is an `int` for a `cob` list
and a `std::size_t` for an `atb` one.
