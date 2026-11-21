# The timer modules

```cpp
#include "cobtmrlist.hpp"       // or "atbtmrlist.hpp"
cobtmrlist<ID, N, U>

#include "cobtmrwheel.hpp"      // or "atbtmrwheel.hpp"
cobtmrwheel<ID, N, U, SPOKES>

#include "cobtmrmill.hpp"       // or "atbtmrmill.hpp"
cobtmrmill<ID, N, U, LVL = 5, DIM = 64, EXACT = false>

#include "cobtmrleanmill.hpp"   // or "atbtmrleanmill.hpp"
cobtmrleanmill<ID, N, U, NULIFY, LVL = 5, DIM = 64>
```

Four ways to keep at most `N` running timers, with the same
interface, which this document describes. None of them reads a clock
or runs a thread: you tell them how much time has passed, and they
tell you which timers have expired. The [README](README.md) says how
to choose between them.

There is no Standard counterpart of any of this.

| Module           | Start    | Stop by index | Expires                         |
|------------------|----------|---------------|---------------------------------|
| `cobtmrlist`     | O(n)     | O(1)          | exactly                         |
| `cobtmrwheel`    | O(n / SPOKES) | O(1)     | exactly, on a tick              |
| `cobtmrmill`     | O(1)     | O(1)          | the longer, the less precisely  |
| exact mill       | O(1)     | O(1)          | exactly, on a tick              |
| `cobtmrleanmill` | O(1)     | O(1)          | the longer, the less precisely  |

Each has variants, named as [common.md](common.md) explains, with
the same template arguments:

* `cobchecked...`: with checked indices, see "Checked indices" below.
* `cobhive...` and `cobcheckedhive...`: see "A bit for each timer"
  below. The lean mill has no such variant.
* `atb...`: the number of timers is set at run time, see "Number of
  timers set at run time" below.

The template parameters:

* `ID` is what identifies a timer to you, see below.
* `N` is the number of timers there is room for.
* `U` is the type of a duration, see "Time".
* `SPOKES` is the number of spokes of the wheel. It has to be more
  than 1; around 10 is a good choice.
* `LVL` is the number of wheels of a mill and `DIM` the number of
  spokes of each. A mill needs `N` to be more than `LVL * DIM`.
* `EXACT` makes the mill exact, see "Exact mill".
* `NULIFY` is the "nulifier" of the lean mill, see the end.

Each module has the member type `tmrID`, which is `ID`.


# Time

`U` is the type that durations are given in. For the wheel and the
mills it has to be a `std::chrono::duration`, such as
`std::chrono::milliseconds` (or a type that is like one: with
`count()`, `++`, comparison and construction from a number). The
timer list only adds, subtracts and compares durations, so there it
can be any type that does that, a plain integer included.

The wheel and the mills work in _ticks_. A tick is one unit of `U`:
with `std::chrono::milliseconds`, a millisecond. If you want a
coarser tick, use a coarser `U` (a `std::chrono::duration` with the
period you want). The cost of `process_expired()` grows with the
number of ticks that have passed, as each tick is gone through.

The timer list has no ticks: durations are what they are.


# Room for timers

A timer list has room for `N` timers.

A wheel shares its room out between the spokes: each spoke has room
for `N / SPOKES * 2` timers, twice its even share. A timer goes to
the spoke that its duration picks, so the wheel can refuse a timer
with fewer than `N` running, if too many of them are in the same
spoke.

A regular mill and a lean mill do the same: each of the `LVL * DIM`
spokes has room for `N / (LVL * DIM) + 1` timers.

The exact mill does not: all its spokes use the same room, so `N`
timers can be running whatever their durations are.


# The longest duration

```cpp
static constexpr U max_duration;        // the mills only
```

A mill can't keep a timer for longer than `DIM` to the power of `LVL`
ticks, less one. That is `max_duration`. With the defaults, 5 wheels
of 64 spokes, it is over a thousand million ticks.

The timer list and the wheel have no such limit.


# Identifying a timer

A timer is identified by an `ID`, which is type a template parameter
for all timer classes. This type is assumed to have value-semantics.
So, a copy of a given ID will be kept with an active/running timer,
thus take care to not have an extremely large ID.

An `ID` has to be default constructible and copyable, and comparable
with `==`, which is how a timer is found when it is stopped by ID.


# Start a timer

```cpp
index start(ID id, U duration);
```

Starts a timer with the ID `id` that will timeout in `duration`.

If a timer is started succesfully, a valid index is returned, which
can be used to stop the timer in an efficient manner (but with 
a risk that things can go wrong if this timer expires by the time
you stop the timer).

If a timer is not started, a special invalid index is returned. The
value of this "invalid" index is not specified and there is no
specified way to check an index for validity by the user.

A timer is not started if there is no room for it (see "Room for
timers") and, in the mills, if its duration is negative or longer
than `max_duration`. It is always safe to stop by the index that was
returned, whether the timer was started or not.

The index of a mill does have a way to be asked:

```cpp
bool index::valid() const;      // the mills only
```

which tells whether the timer was started.

In the wheel, a duration of less than a tick is taken to be one tick.
In the regular mill a timer of the first wheel, and in the exact mill
every timer, expires on the tick after its duration has passed: a
timer of 5 ticks is reported by the sixth tick.


# Stop a timer by ID

```cpp
int stop(ID id);
```

Stops a running/active timer that has ID of `id`.

If a timer is found and stopped succesfully, returns 0; or -1
otherwise.

If two or more timers timers are active with this same ID, it is not
specified which one will be stopped (but _one_ of them _will_ be
stopped).

In general, timers that are to expire sooner will be check before the
ones that are later to expire, but there is no specified order of
checking timers for the given ID.

This is slower than stopping by index, but safe.


# Stop a timer by index

```cpp
void stop(index it);
```

Stops a timer with the index `it`.

While some timer modules/classes might return a value, in general,
it is not specified that this returns a value. There is no guaranteed
indication if a timer was actually stopped.

As it is: the timer list and the wheel return an `int`, 0 if a timer
was stopped and -1 if there was none at the index. The mills return
nothing.

For most timers, this will remove the timer. But, for some, which
requira a "nullifying" function, said function would be used and it is
the responsibility of the user to ignore the nullified timers on expiry.

This is _very_ fast (O(1)). If no timer is active with this index (for
example, because it was already stopped), this does nothing, unless a
new timer was started in the meantime and got the same slot, in which
case that new timer is stopped.


# Process expired timers

```cpp
template <class F> void process_expired(U elapsed, F f)
```

Will call `f` with the ID of all timers that expired since the last
call of this function which is taken to have happened `elapsed` time
ago. If none have expired, `f` will not be called even once.

There may be more than one timer expired with the same `ID`.

It is fine to start and stop timers from `f`. A timer is not running any
more when `f` is called for it, so do not stop it by index from `f` (that
index might already belong to some other timer).

For timer which are nullified instead of removed on stopping, the `ID`
passed would be that of a "nullified" timer, which is opaque to the
timer module/class, so it's up to the user to recognize a nullified
timer and ignore its expiry.


# Stop (and get) the next timer to expire

```cpp
template <template <class> class V> V<ID> stop_first()
```

Stops the timer that would expire first, given enough time and returns
its ID, unless there are no active timers.

The result looks scary, but it's just a "generic" optional. That is,
we did not want to tie this to `std::optional` - user can choose
any class that conforms to that interface in as much as we use it.

Example:

```cpp
    auto id = queue.stop_first<std::optional>();
    if (id.has_value()) {
        process(*id);
    }
```

For example, if we just want to stop the first timer if there is one
and don't care about its ID, we can:
	
```cpp
    template <class U> struct ignore {
        ignore() {}
        ignore(U const&) {}
    };
	timers.stop_first<ignore>();
    
```

As you can see, this illustrates the "concept" that we use - we need a
default constructor to mean "no value" and a constructor that accepts
the `ID` and saves it, indicating "has value". 

If several timers would expire at the same time, it is not specified
which of them is stopped.

The timer list does this in O(1). The wheel looks at each of its
spokes, and the mills may look at all of theirs; the exact mill looks
at all the timers.


# Is any timer running

```cpp
bool empty() const;     // the timer list only
```

Whether there are no running timers. Only the timer list has this.


# Exact mill

The timer mill takes one more template parameter, `EXACT`, by default
`false`. If it is `true`, timers are cascaded from wheel to wheel as
time passes, so a timer of any duration expires as precisely as the
ones at the first level do: on the tick after its duration.

```cpp
cobtmrmill<ID, N, U, LVL, DIM, true> mill;
```

The interface is the one described here. All `N` timers can be running
whatever their durations are, as the spokes share the room for them.

The index that the exact mill gives for a timer is checked: when that
timer is not running any more, stopping by its index does nothing, even
if a new timer was started since and got the same place.


# Checked indices

```cpp
cobcheckedtmrlist<ID, N, U>
cobcheckedtmrwheel<ID, N, U, SPOKES>
cobcheckedtmrmill<ID, N, U, LVL, DIM, EXACT>
cobcheckedtmrleanmill<ID, N, U, NULIFY, LVL, DIM>
```

In the modules as they are, the index of a timer that is not running
any more - it has expired, or was stopped - may get to be the index
of another timer, started since. Stopping by an index that old stops
that other timer.

In the `checked` variants it does not: such an index stays the index
of nothing, and stopping by it does nothing, however many timers were
started since. The interface is the same.

For the list, the wheel and the regular mill, that takes an
`unsigned` for each timer. For the lean mill it takes an `unsigned`
for each spoke: an index is good until the spoke its timer was in
has expired. In all of them the index gets bigger.

The exact mill always has checked indices, so `cobcheckedtmrmill`
with `EXACT` being `true` is the same type as `cobtmrmill` with it.


# A bit for each timer

```cpp
cobhivetmrlist<ID, N, U>                cobcheckedhivetmrlist<ID, N, U>
cobhivetmrwheel<ID, N, U, SPOKES>       cobcheckedhivetmrwheel<ID, N, U, SPOKES>
cobhivetmrmill<ID, N, U, LVL, DIM, EXACT>
cobcheckedhivetmrmill<ID, N, U, LVL, DIM, EXACT>
```

The list, the wheel and the mills (regular and exact) keep their
timers in a pool, which keeps a byte for each slot to tell whether
there is a timer in it. In the `hive` variants that is a bit: an
eighth of the room for that, for a little more work each time a
timer is got at. See "Hive" in [common.md](common.md).

Nothing else changes: the same timers expire at the same time and
are reported in the same order.

The lean mill does not keep its timers in a pool, so there is no
such variant of it.


# Number of timers set at run time

Each timer module has an `atb` variant, where the number of timers is
the dimension of a tag (see [atbdim.md](atbdim.md)) rather than `N`:
`atbtmrlist<ID, Tag, U>`, `atbtmrwheel<ID, Tag, U, SPOKES>`,
`atbtmrmill<ID, Tag, U, LVL, DIM, EXACT>` and
`atbtmrleanmill<ID, Tag, U, NULIFY, LVL, DIM>`. The interface is the same.

So do the variants: `atbcheckedtmrlist`, `atbhivetmrwheel`,
`atbcheckedhivetmrmill` and so on.

The room is shared out as for the `cob` ones, with the dimension for
`N`. Where a `cob` mill does not compile for having no more timers
than spokes, an `atb` mill of such a dimension has no room at all,
and starts nothing. Nothing is allocated after a timer module is
made.


# Nulifier - for lean timers

Lean timers reduce memory footprint by using a "nulifier" function
object that invalidates a timer ID. So, a timer that is cancelled is
actually "nullified", it is not removed from the list.

For this to work correctly the nullifier has to obey a contract:

```cpp
class nulify {
    bool operator()(ID& id) {
        if (id.valid()) {
            id.invalidate();
            return true;
        }
        return false;
    }
};
```

Of course, `nulify` does not have to be implemented in exactly that
way, but it's the "canonical" way to do it. The function object should
check if the ID is valid and only then invalidate and return `true`.
If the ID is already invalid, just return `false`.

It's up to the user to provide the `valid()` and `invalidate()`
functions (which need not be named like that, not even be actual
functions). Commonly, they check and set some special value, like `-1`
for integers, `NaN` for floating point, `NUL` character, etc.
