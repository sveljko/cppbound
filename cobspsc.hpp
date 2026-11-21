/* Compile time bound queue between two threads. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBSPSC)
#define      INC_COBSPSC


#include "cobbound.hpp"

#include <atomic>
#include <utility>


// A queue for one producer and one consumer, each of which may be a thread
// (or an interrupt handler) of its own. There are no locks and no waiting:
// each operation takes a few steps, whatever the other side is doing.
//
// That there is only one of each is seen to: putting in is done through
// the `producer` and taking out through the `consumer`, and the queue gives
// each of them out only once - until it is given back with `release()`.
// They can be moved (to the thread that is to use them), but not copied. Like an iterator, they are of use only while
// the queue is there.
template <class T, class B>
struct bndspsc {
    using value_type = T;
    using size_type  = typename B::size_type;

    bndspsc()                          = default;
    bndspsc(bndspsc const&)            = delete;
    bndspsc& operator=(bndspsc const&) = delete;
    // When both sides are done with it
    ~bndspsc()
    {
        while (drop_front()) {
        }
    }

    // The side that puts in. One that was moved from puts nothing in.
    class producer {
        bndspsc* q;

        explicit producer(bndspsc* q_) : q(q_) {}
        friend struct bndspsc;

    public:
        producer(producer&& x) noexcept : q(std::exchange(x.q, nullptr)) {}
        producer& operator=(producer&& x) noexcept
        {
            q = std::exchange(x.q, nullptr);
            return *this;
        }

        // false if there is no room
        template <class... A> bool emplace(A&&... a) { return (q != nullptr) && q->emplace(std::forward<A>(a)...); }
        bool push(T const& t) { return emplace(t); }
        bool push(T&& t) { return emplace(std::move(t)); }
        // If it is not, it stays so until this side puts something in
        bool full() const noexcept { return (q == nullptr) || q->full(); }
    };

    // The side that takes out. One that was moved from finds nothing.
    class consumer {
        bndspsc* q;

        explicit consumer(bndspsc* q_) : q(q_) {}
        friend struct bndspsc;

    public:
        consumer(consumer&& x) noexcept : q(std::exchange(x.q, nullptr)) {}
        consumer& operator=(consumer&& x) noexcept
        {
            q = std::exchange(x.q, nullptr);
            return *this;
        }

        // The oldest element, taken out, or nothing
        template <template <class> class V> V<T> pop() { return (q != nullptr) ? q->template pop<V>() : V<T>{}; }
        // Calls f with the oldest element, if there is one, and leaves it
        // in. Returns whether it did.
        template <class F> bool with_front(F&& f) { return (q != nullptr) && q->with_front(std::forward<F>(f)); }
        // Takes the oldest element out. false if there is none.
        bool drop_front() { return (q != nullptr) && q->drop_front(); }
        // If it is not, it stays so until this side takes something out
        bool empty() const noexcept { return (q == nullptr) || q->empty(); }
    };

    // The producer, the first time this is called; nothing after that
    template <template <class> class V> V<producer> claim_producer()
    {
        return produced.exchange(true) ? V<producer>{} : V<producer>(producer{ this });
    }
    // The consumer, the first time this is called; nothing after that
    template <template <class> class V> V<consumer> claim_consumer()
    {
        return consumed.exchange(true) ? V<consumer>{} : V<consumer>(consumer{ this });
    }

    // Gives a side back, so that it can be claimed again (say, by a thread
    // that takes over). Only this does that: a side that is just destroyed
    // is not given back. false, and nothing changes, if it is not a side of
    // this queue (or was moved from).
    bool release(producer&& p)
    {
        if (p.q != this) {
            return false;
        }
        p.q = nullptr;
        produced.store(false);
        return true;
    }
    bool release(consumer&& c)
    {
        if (c.q != this) {
            return false;
        }
        c.q = nullptr;
        consumed.store(false);
        return true;
    }

    // These may be asked by anyone. While the two sides are at it, they tell
    // how it was a moment ago.
    size_type size() const noexcept
    {
        return between(head.load(std::memory_order_acquire), tail.load(std::memory_order_acquire));
    }
    size_type max_size() const noexcept { return B::size(); }
    bool      empty() const noexcept { return size() == 0; }
    bool      full() const noexcept { return size() == B::size(); }

private:
    using slot = typename B::slot;

    // The producer only
    template <class... A> bool emplace(A&&... a)
    {
        const size_type to   = tail.load(std::memory_order_relaxed);
        const size_type from = head.load(std::memory_order_acquire);
        slot            s;
        if ((between(from, to) == B::size()) || !s.be(place(to))) {
            return false;
        }
        store.make(s, std::forward<A>(a)...);
        tail.store(after(to), std::memory_order_release);
        return true;
    }
    // The consumer only (and the destructor)
    template <template <class> class V> V<T> pop()
    {
        const size_type from = head.load(std::memory_order_relaxed);
        slot            s;
        if ((from == tail.load(std::memory_order_acquire)) || !s.be(place(from))) {
            return {};
        }
        V<T> rslt(std::move(store.at(s)));
        store.drop(s);
        head.store(after(from), std::memory_order_release);
        return rslt;
    }
    template <class F> bool with_front(F&& f)
    {
        const size_type from = head.load(std::memory_order_relaxed);
        slot            s;
        if ((from == tail.load(std::memory_order_acquire)) || !s.be(place(from))) {
            return false;
        }
        f(store.at(s));
        return true;
    }
    bool drop_front()
    {
        const size_type from = head.load(std::memory_order_relaxed);
        slot            s;
        if ((from == tail.load(std::memory_order_acquire)) || !s.be(place(from))) {
            return false;
        }
        store.drop(s);
        head.store(after(from), std::memory_order_release);
        return true;
    }

    // `head` and `tail` count the elements taken out and put in, going
    // around after twice the capacity, so that all slots can be in use and
    // "full" is still not the same as "empty".
    static size_type after(size_type i) { return (i + 1 == 2 * B::size()) ? 0 : i + 1; }
    static size_type place(size_type i) { return (i < B::size()) ? i : i - B::size(); }
    static size_type between(size_type from, size_type to)
    {
        return (to >= from) ? to - from : to + (2 * B::size() - from);
    }

    typename B::template store<T> store;
    // Apart, so that the two sides do not slow each other down
    alignas(64) std::atomic<size_type> head{ 0 };     // written by the consumer only
    alignas(64) std::atomic<size_type> tail{ 0 };     // written by the producer only
    std::atomic<bool>                  produced{ false };
    std::atomic<bool>                  consumed{ false };
};

template <class T, int N> using cobspsc = bndspsc<T, cobbound<N>>;


#endif // !defined(INC_COBSPSC)
