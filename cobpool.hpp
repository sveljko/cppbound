/* Compile time bound object pool. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBPOOL)
#define      INC_COBPOOL


#include "cobarray.hpp"
#include "cobstore.hpp"

#include <utility>


// A fixed number of slots; objects are made and dropped one by one and are
// referred to by a handle. A handle of a dropped object is harmless (but if
// a new object took the slot, it refers to that one).
template <class T, int N>
struct cobpool {
    static_assert(N > 0, "Pool must have some capacity");

    using value_type = T;
    using handle     = cobint<0, N>;
    using index      = cobint<0, N - 1>;

    static constexpr auto nil = cobic<N>;

    cobpool() : vacant(cobic<0>)
    {
        int i = 1;
        for (auto& x : next) {
            if (!x.be(i++)) {
                x = nil;
            }
        }
        used.fill(false);
    }
    cobpool(cobpool const& x) : next(x.next), used(x.used), vacant(x.vacant), count(x.count)
    {
        copy_objects(x);
    }
    cobpool& operator=(cobpool const& x)
    {
        if (this != &x) {
            clear();
            next   = x.next;
            used   = x.used;
            vacant = x.vacant;
            count  = x.count;
            copy_objects(x);
        }
        return *this;
    }
    ~cobpool() { clear(); }

    // `nil` if there is no room
    template <class... A> handle make(A&&... a)
    {
        if (nil == vacant) {
            return nil;
        }
        index i;
        i.be(vacant.get());
        const handle h = vacant;
        vacant         = next.get(i);
        used.set(i, true);
        ++count;
        store.make(i, std::forward<A>(a)...);
        return h;
    }
    // false if there is no object at `h`
    bool drop(handle h)
    {
        index i;
        if (!i.be(h.get()) || !used.get(i)) {
            return false;
        }
        store.drop(i);
        used.set(i, false);
        next.set(i, vacant);
        vacant = h;
        --count;
        return true;
    }
    bool live(handle h) const
    {
        index i;
        return i.be(h.get()) && used.get(i);
    }

    template <class F> bool with(handle h, F&& f)
    {
        index i;
        if (i.be(h.get()) && used.get(i)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    template <class F> bool with(handle h, F&& f) const
    {
        index i;
        if (i.be(h.get()) && used.get(i)) {
            f(store.at(i));
            return true;
        }
        return false;
    }
    // A copy, or `T{}` if there is no object
    T get(handle h) const
    {
        index i;
        return (i.be(h.get()) && used.get(i)) ? store.at(i) : T{};
    }

    // In index order
    template <class F> void for_each(F&& f)
    {
        for (auto i : next.irange()) {
            if (used.get(i)) {
                f(store.at(i));
            }
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (auto i : next.irange()) {
            if (used.get(i)) {
                f(store.at(i));
            }
        }
    }

    unsigned size() const noexcept { return count; }
    unsigned max_size() const noexcept { return N; }
    bool     empty() const noexcept { return count == 0; }
    bool     full() const noexcept { return count == N; }

    void clear()
    {
        for (auto i : next.irange()) {
            if (used.get(i)) {
                drop(i);
            }
        }
    }

private:
    void copy_objects(cobpool const& x)
    {
        for (auto i : next.irange()) {
            if (used.get(i)) {
                store.make(i, x.store.at(i));
            }
        }
    }

    cobstore<T, N>    store;
    cobarray<handle, N> next;
    cobarray<bool, N>   used;
    handle              vacant;
    unsigned            count = 0;
};


#endif // !defined(INC_COBPOOL)
