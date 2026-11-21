#include "atbspsc.hpp"
#include "cobspsc.hpp"

#include "atbarray.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct five {};     // the tag of the dimension, which is 5
struct late {};     // set by a thread, while others look

static_assert(!std::is_copy_constructible_v<cobspsc<int, 4>>);
static_assert(!std::is_copy_assignable_v<atbspsc<int, five>>);

// Each side of the queue is given out once
static_assert(!std::is_copy_constructible_v<cobspsc<int, 4>::producer>);
static_assert(!std::is_copy_constructible_v<cobspsc<int, 4>::consumer>);
static_assert(std::is_move_constructible_v<cobspsc<int, 4>::producer>);
static_assert(!std::is_default_constructible_v<cobspsc<int, 4>::consumer>);

template <class Q> void basics(unsigned capacity)
{
    Q    q;
    auto claimed_tx = q.template claim_producer<std::optional>();
    auto claimed_rx = q.template claim_consumer<std::optional>();
    CHECK(claimed_tx.has_value() && claimed_rx.has_value());
    // only once
    CHECK(!q.template claim_producer<std::optional>().has_value());
    CHECK(!q.template claim_consumer<std::optional>().has_value());
    auto& tx = *claimed_tx;
    auto& rx = *claimed_rx;

    CHECK(q.empty() && !q.full() && q.size() == 0 && q.max_size() == capacity && rx.empty() && !tx.full());
    CHECK(!rx.template pop<std::optional>().has_value() && !rx.drop_front());
    CHECK(!rx.with_front([](std::string&) { CHECK(false); }));

    for (unsigned i = 0; i < capacity; ++i) {
        CHECK(tx.push(std::to_string(i)) && q.size() == i + 1);
    }
    CHECK(q.full() && tx.full() && !tx.push("x") && !tx.emplace(3u, 'x') && q.size() == capacity);

    std::string front;
    CHECK(rx.with_front([&](std::string& s) { front = s; }) && front == "0" && q.size() == capacity);
    CHECK(rx.template pop<std::optional>() == "0" && rx.drop_front() && q.size() == capacity - 2);
    CHECK(tx.emplace(2u, 'y') && tx.push(std::string("z")) && q.full());

    // around the end of the storage, many times
    for (unsigned i = 0; i < 10 * capacity + 3; ++i) {
        const auto s = rx.template pop<perhaps>();
        CHECK(s.has_value() && tx.push(std::to_string(i)) && q.full());
    }
    for (unsigned i = 0; i < capacity; ++i) {
        CHECK(rx.template pop<std::optional>() == std::to_string(9 * capacity + 3 + i));
    }
    CHECK(q.empty() && rx.empty() && !rx.template pop<std::optional>().has_value());

    // A side can be moved to where it is used; what it was moved from does nothing
    auto tx2 = std::move(tx);
    auto rx2 = std::move(rx);
    CHECK(!tx.push("a") && tx.full() && rx.empty() && !rx.template pop<std::optional>().has_value() && !rx.drop_front());
    CHECK(!rx.with_front([](std::string&) { CHECK(false); }));
    CHECK(tx2.push("a") && rx2.template pop<std::optional>() == "a");
    tx = std::move(tx2);
    CHECK(tx.push("b") && !tx2.push("c") && q.size() == 1);
    CHECK(!q.template claim_producer<std::optional>().has_value());     // still only once
}

// One thread puts in, another takes out: everything arrives, once, in order
template <class Q> void twothreads()
{
    constexpr int total = 200000;
    Q             q;
    auto          rx = q.template claim_consumer<std::optional>();
    std::thread   producer([tx = *q.template claim_producer<std::optional>()]() mutable {
        for (int i = 0; i < total;) {
            if (tx.push(i)) {
                ++i;
            }
        }
    });
    long long sum = 0;
    for (int i = 0; i < total;) {
        if (const auto x = rx->template pop<std::optional>()) {
            CHECK(*x == i);
            sum += *x;
            ++i;
        }
    }
    producer.join();
    CHECK(q.empty() && sum == static_cast<long long>(total) * (total - 1) / 2);
}

// A side that is given back can be claimed again; one that is just gone can not
template <class Q> void releasing()
{
    Q q;
    Q other;
    {
        auto tx = q.template claim_producer<std::optional>();
        CHECK(tx->push(1));
    }
    CHECK(!q.template claim_producer<std::optional>().has_value());     // destroyed is not given back

    auto rx = *q.template claim_consumer<std::optional>();
    CHECK(rx.template pop<std::optional>() == 1);
    auto theirs = *other.template claim_consumer<std::optional>();
    CHECK(!q.release(std::move(theirs)));        // not of this queue: nothing changes
    CHECK(!q.template claim_consumer<std::optional>().has_value());
    CHECK(other.release(std::move(theirs)) && !other.release(std::move(theirs)));   // the second time it is of no queue

    CHECK(q.release(std::move(rx)));
    CHECK(rx.empty() && !rx.template pop<std::optional>().has_value() && !q.release(std::move(rx)));
    auto rx2 = q.template claim_consumer<std::optional>();
    CHECK(rx2.has_value() && !q.template claim_consumer<std::optional>().has_value());

    // what is in the queue stays there for whoever takes over
    auto otx = *other.template claim_producer<std::optional>();
    CHECK(otx.push(7) && other.release(std::move(otx)) && !otx.push(8));
    auto otx2 = *other.template claim_producer<std::optional>();
    auto orx  = *other.template claim_consumer<std::optional>();
    CHECK(otx2.push(9) && orx.template pop<std::optional>() == 7 && orx.template pop<std::optional>() == 9);
}

// Many threads ask for a side: one gets it
void claiming()
{
    cobspsc<int, 4>          q;
    std::atomic<int>         got{ 0 };
    std::atomic<bool>        go{ false };
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            while (!go.load()) {
            }
            got += q.claim_producer<std::optional>().has_value();
            got += 10 * q.claim_consumer<std::optional>().has_value();
        });
    }
    go.store(true);
    for (auto& t : threads) {
        t.join();
    }
    CHECK(got == 11);
}

// What is still in when the queue goes is destroyed, and nothing else is
void lifetimes()
{
    counted::alive = 0;
    {
        cobspsc<counted, 4> q;
        auto                tx = *q.claim_producer<std::optional>();
        auto                rx = *q.claim_consumer<std::optional>();
        CHECK(counted::alive == 0);
        CHECK(tx.emplace(1) && tx.emplace(2) && tx.emplace(3));
        CHECK(counted::alive == 3);
        rx.pop<perhaps>();
        CHECK(counted::alive == 2 && rx.drop_front() && counted::alive == 1);
        CHECK(tx.emplace(4) && tx.emplace(5));
    }
    CHECK(counted::alive == 0);

    cobspsc<std::unique_ptr<int>, 2> up;
    auto                             tx = *up.claim_producer<std::optional>();
    auto                             rx = *up.claim_consumer<std::optional>();
    CHECK(tx.push(std::make_unique<int>(5)) && tx.emplace(new int(6)) && !tx.push(std::make_unique<int>(7)));
    const auto p = rx.pop<std::optional>();
    CHECK(p.has_value() && **p == 5);
}

// The dimension is the same to whoever looks at it, from whatever thread:
// either the one that sets it is the first, or it stays 0 and can't be set
void dimension()
{
    std::atomic<bool>        go{ false };
    std::vector<std::size_t> seen(8, 99);
    std::vector<std::thread> threads;
    std::atomic<int>         set{ 0 };
    for (std::size_t i = 0; i < seen.size(); ++i) {
        threads.emplace_back([&, i] {
            while (!go.load()) {
            }
            if (i == 3) {
                set += atbdim<late>::set(7);
            }
            atbarray<int, late> a;
            seen[i] = a.size();
        });
    }
    go.store(true);
    for (auto& t : threads) {
        t.join();
    }
    const std::size_t is = atbdim<late>::get();
    CHECK((is == 7 && set == 1) || (is == 0 && set == 0));
    for (std::size_t n : seen) {
        CHECK(n == is);
    }
    CHECK(!atbdim<late>::set(9));
}

int main()
{
    CHECK(atbdim<five>::set(5));

    basics<cobspsc<std::string, 4>>(4);
    basics<cobspsc<std::string, 3>>(3);
    basics<atbspsc<std::string, five>>(5);
    twothreads<cobspsc<int, 64>>();
    twothreads<cobspsc<int, 1>>();
    twothreads<atbspsc<int, five>>();
    releasing<cobspsc<int, 4>>();
    releasing<atbspsc<int, five>>();
    claiming();
    lifetimes();
    dimension();

    struct unset {};
    atbspsc<int, unset> none;
    auto                tx = *none.claim_producer<std::optional>();
    auto                rx = *none.claim_consumer<std::optional>();
    CHECK(!tx.push(1) && none.empty() && none.full() && !rx.pop<std::optional>().has_value());
    return 0;
}
