# A queue between two threads

```cpp
#include "cobspsc.hpp"      // or "atbspsc.hpp"

cobspsc<T, N>               atbspsc<T, Tag>
```

A queue of at most `N` elements for one _producer_, which puts
elements in, and one _consumer_, which takes them out in the order
they were put in. The two may be different threads, or one of them
may be an interrupt handler.

There are no locks and no waiting. Each operation takes a few steps
and then returns, whatever the other side is doing at the time: if
there is no room, or no element, it says so right away. So neither
side can ever hold the other one up, which is what makes the queue
fit for an interrupt handler and for a thread that must not be
late.

There is no Standard counterpart.

`T` can be of any type; see [common.md](common.md) for that, and for
the generic optional `V`.

This is the only container in the library that is meant to be used
from more than one thread at a time. For a queue within one thread,
[cobring.md](cobring.md) is simpler and offers more.


# The two sides

The queue only works if there is one producer and one consumer.
Rather than leave that to you, the queue sees to it: an element can
only be put in through a `producer` object and taken out through a
`consumer` object, and the queue gives out one of each.

```cpp
cobspsc<sample, 64> q;
auto tx = q.claim_producer<std::optional>();    // for the interrupt handler
auto rx = q.claim_consumer<std::optional>();    // for the main loop

// in the interrupt handler:
if (!tx->push(s)) { ++dropped; }

// in the main loop:
while (auto s = rx->pop<std::optional>()) { process(*s); }
```

A side can be moved, say into the thread that is to use it, but not
copied. One that was moved from is of no queue: it puts nothing in
and finds nothing. A side has no default constructor.

Like an iterator, a side refers to its queue, so it is of use only
while the queue is there. The queue has to outlive both of them, and
can't itself be copied or moved.

Each side is to be used by one thread at a time. Handing a side over
from one thread to another is up to you to do safely (see
`release()`).


# Claim a side

```cpp
template <template <class> class V> V<producer> claim_producer();
template <template <class> class V> V<consumer> claim_consumer();
```

Returns the producer (or the consumer) of the queue in a generic
optional, if it is not claimed already. If it is, returns nothing.

So of any number of callers, from any number of threads, only one
gets each side.

A side that is destroyed is not given back by that: the queue can't
know that nobody else has got hold of it. Once claimed, a side stays
claimed until it is released.


# Give a side back

```cpp
bool release(producer&& p);
bool release(consumer&& c);
```

Gives a side back to the queue, so that it can be claimed again -
say, by a thread that takes the place of one that is done. `p` (or
`c`) is left as one that was moved from. What is in the queue stays
there.

Returns `true` if the side was given back. Returns `false`, and
changes nothing, if it is not a side of this queue: if it is of
another queue, or was moved from.

```cpp
q.release(std::move(*tx));
auto tx2 = q.claim_producer<std::optional>();   // has a value again
```

Whoever releases a side must be done using it. The queue makes sure
that there is one producer object; that the thread that had it has
stopped using it before the next one starts is for you to see to.


# The producer

```cpp
template <class... A> bool emplace(A&&... a);
bool push(T const& t);
bool push(T&& t);
bool full() const noexcept;
```

`emplace()` constructs a new element from `a...` at the back of the
queue; `push()` copies or moves `t` there. They return `false`, and
do nothing, if there is no room.

`full()` tells whether there is no room. For the producer this is
more than a guess: if the queue is not full, it stays so until the
producer itself puts something in, as the consumer only makes room.
So after `full()` returned `false`, the next `push()` succeeds.

A producer that was moved from, or released, puts nothing in, and is
always `full()`.


# The consumer

```cpp
template <template <class> class V> V<T> pop();
template <class F> bool with_front(F&& f);
bool drop_front();
bool empty() const noexcept;
```

`pop<V>()` takes the oldest element out and returns it, moved into a
generic optional. Returns nothing if the queue is empty.

`with_front()` calls `f` with a reference to the oldest element,
leaves it in the queue, and returns `true`. If the queue is empty, `f`
is not called and `false` is returned.

`drop_front()` takes the oldest element out and destroys it. Returns
`false` if there is none.

The last two together are for an element that should leave the queue
only after it was dealt with, or that can't be moved:

```cpp
bool sent = false;
if (rx->with_front([&](message& m) { sent = send(m); }) && sent) {
    rx->drop_front();
}
```

`empty()` tells whether there is nothing to take out. As with
`full()` of the producer, if the queue is not empty, it stays so
until the consumer itself takes something out.

A consumer that was moved from, or released, finds nothing, and is
always `empty()`.


# Ask the queue

```cpp
size_type size() const noexcept;
size_type max_size() const noexcept;
bool      empty() const noexcept;
bool      full() const noexcept;
```

These are members of the queue itself, and can be called by anyone,
from any thread.

While the two sides are at work, what `size()`, `empty()` and
`full()` tell is how it was a moment ago: by the time you look at the
answer, it may have changed. They are good for statistics and for
tests, not for deciding whether a `push()` or a `pop()` will succeed -
for that, each side has its own `full()` and `empty()`.

`max_size()` is `N`. All `N` slots can be in use.


# When the queue is destroyed

The elements that are still in it are destroyed. Both sides have to
be done with the queue by then.


# What is guaranteed, and what is not

* Each element that was put in is taken out once, and they come out
  in the order they went in.
* An operation of one side never waits for the other: it is
  "wait-free".
* Nothing is allocated (for `atbspsc`: after the queue is made).
* It is not a queue for several producers, or several consumers.
  Two threads must not use the same side at the same time.
* The two sides are kept apart in memory (each on a cache line of
  its own), so that they do not slow each other down. That makes the
  queue object about two cache lines bigger than the room for its
  elements.
