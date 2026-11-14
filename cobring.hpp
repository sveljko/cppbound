/* Compile time bound ring buffer. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBRING)
#define      INC_COBRING


#include "cobstore.hpp"

#include <utility>


// First in, first out, capacity N
template <class T, int N>
struct cobring {
    static_assert(N > 0, "Ring must have some capacity");

    using value_type = T;
    using index      = cobint<0, N - 1>;

    cobring() = default;
    cobring(cobring const& x)
    {
        for (unsigned i = 0; i < x.count; ++i) {
            emplace_back(x.store.at(x.slot(i)));
        }
    }
    cobring& operator=(cobring const& x)
    {
        if (this != &x) {
            clear();
            for (unsigned i = 0; i < x.count; ++i) {
                emplace_back(x.store.at(x.slot(i)));
            }
        }
        return *this;
    }
    ~cobring() { clear(); }

    template <class... A> bool emplace_back(A&&... a)
    {
        if (count == N) {
            return false;
        }
        store.make(slot(count), std::forward<A>(a)...);
        ++count;
        return true;
    }
    bool push_back(T const& t) { return emplace_back(t); }
    bool push_back(T&& t) { return emplace_back(std::move(t)); }

    // If full, drops the oldest first, so it always succeeds
    template <class... A> void emplace_over(A&&... a)
    {
        if (count == N) {
            drop_front();
        }
        emplace_back(std::forward<A>(a)...);
    }

    template <template <class> class V> V<T> pop_front()
    {
        if (count == 0) {
            return {};
        }
        V<T> rslt(std::move(store.at(head)));
        drop_front();
        return rslt;
    }
    template <template <class> class V> V<T> front() const
    {
        return (count == 0) ? V<T>{} : V<T>(store.at(head));
    }
    template <template <class> class V> V<T> back() const
    {
        return (count == 0) ? V<T>{} : V<T>(store.at(slot(count - 1)));
    }
    // The i-th oldest
    template <template <class> class V> V<T> at(unsigned i) const
    {
        return (i >= count) ? V<T>{} : V<T>(store.at(slot(i)));
    }
    template <class F> bool with_front(F&& f)
    {
        if (count == 0) {
            return false;
        }
        f(store.at(head));
        return true;
    }

    // Oldest to newest
    template <class F> void for_each(F&& f)
    {
        for (unsigned i = 0; i < count; ++i) {
            f(store.at(slot(i)));
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (unsigned i = 0; i < count; ++i) {
            f(store.at(slot(i)));
        }
    }

    unsigned size() const noexcept { return count; }
    unsigned max_size() const noexcept { return N; }
    bool     empty() const noexcept { return count == 0; }
    bool     full() const noexcept { return count == N; }

    void clear()
    {
        while (count > 0) {
            drop_front();
        }
        head = index{};
    }

private:
    // The slot of the i-th oldest element, `i` in [0, N)
    index slot(unsigned i) const
    {
        index rslt;
        rslt.be(static_cast<int>((static_cast<unsigned>(head.get()) + i) % static_cast<unsigned>(N)));
        return rslt;
    }
    void drop_front()
    {
        store.drop(head);
        head.advance_wrap();
        --count;
    }

    cobstore<T, N> store;
    index          head;
    unsigned       count = 0;
};


#endif // !defined(INC_COBRING)
