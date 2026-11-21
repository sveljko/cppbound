/* Compile time bound ring buffer. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBRING)
#define      INC_COBRING


#include "cobbound.hpp"

#include <utility>


// First in, first out, capacity N
template <class T, class B>
struct bndring {
    using value_type = T;
    using size_type  = typename B::size_type;
    using index      = typename B::slot;

    bndring() = default;
    bndring(bndring const& x)
    {
        for (size_type i = 0; i < x.count; ++i) {
            emplace_back(x.store.at(x.slot(i)));
        }
    }
    bndring& operator=(bndring const& x)
    {
        if (this != &x) {
            clear();
            for (size_type i = 0; i < x.count; ++i) {
                emplace_back(x.store.at(x.slot(i)));
            }
        }
        return *this;
    }
    ~bndring() { clear(); }

    template <class... A> bool emplace_back(A&&... a)
    {
        if (count == B::size()) {
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
        if (count == B::size()) {
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
    template <template <class> class V> V<T> at(size_type i) const
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
        for (size_type i = 0; i < count; ++i) {
            f(store.at(slot(i)));
        }
    }
    template <class F> void for_each(F&& f) const
    {
        for (size_type i = 0; i < count; ++i) {
            f(store.at(slot(i)));
        }
    }

    size_type size() const noexcept { return count; }
    size_type max_size() const noexcept { return B::size(); }
    bool     empty() const noexcept { return count == 0; }
    bool     full() const noexcept { return count == B::size(); }

    void clear()
    {
        while (count > 0) {
            drop_front();
        }
        head = index{};
    }

private:
    // The slot of the i-th oldest element, `i` less than the capacity
    index slot(size_type i) const
    {
        index rslt;
        if (B::size() > 0) {
            rslt.be((static_cast<size_type>(head.get()) + i) % B::size());
        }
        return rslt;
    }
    void drop_front()
    {
        store.drop(head);
        head.advance_wrap();
        --count;
    }

    typename B::template store<T> store;
    index                         head;
    size_type                     count = 0;
};

template <class T, int N> using cobring = bndring<T, cobbound<N>>;


#endif // !defined(INC_COBRING)
