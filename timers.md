# Start a timer

```
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


# Stop a timer by ID

```
int stop(ID id);
```

Stops a timer with the ID `id`.

If a timer is found and stopped succesfully, returns 0; or -1
otherwise.

If two timers are active with this same ID, it is not specified which
one will be stopped.

This is slower than stopping by index, but safe.


# Stop a timer by index

```
void stop(index it);
```

Stops a timer with the index `it`.

While some timer modules/classes might return a value, in general,
it is not specified that this returns a value. There is no guaranteed
indication if a timer was actually stopped.

For most timers, this will remove the timer. But, for some, which
requira a "nullifying" function, said function would be used and it is
the responsibility of the user to ignore the nullified timers on expiry.

While this should be _very_ fast (O(1)), if no timer is active with
this index, the outcome might corrupt the timers, so users needs to be
sure that the timer is still active when calling this. Some timers
might be able to check if the timer is active, and make this safe, if
slightly slower, but this is not guaranteed for all timers (for which
a debug mode should be available that catches such errors).


# Process expired timers

```
    template <class F> void process_expired(U elapsed, F f)
```

Will call `f` with the ID of all timers that expired since the last
call of this function which is taken to have happened `elapsed` time
ago. If none have expired, `f` will not be called even once.

There may be more than one timer expired with the same `ID`.

For timer which are nullified instead of removed on stopping, the `ID`
passed would be that of a "nullified" timer, which is opaque to the
timer module/class, so it's up to the user to recognize a nullified
timer and ignore its expiry.


# Stop (and get) the next timer to expire

```
template <template <class> class V> V<ID> stop_first()
```

Stops the timer that would expire first, given enough time and returns
its ID, unless there are no active timers.

The result looks scary, but it's just a "generic" optional. That is,
we did not want to tie this to `std::optional` - user can choose
any class that conforms to that interface in as much as we use it.

Example:

```
	auto id = queue.stop_first<std::optional>();
	if (id.has_value()) {
	    process(*id);
	}
```

For example, if we just want to stop the first timer if there is one
and don't care about its ID, we can:
	
```
    template <class U> struct ignore {
        ignore() {}
        ignore(U const&) {}
    };
	timers.stop_first<ignore>();
    
```

As you can see, this illustrates the "concept" that we use - we need a
default constructor to mean "no value" and a constructor that accepts
the `ID` and saves it, indicating "has value". 
