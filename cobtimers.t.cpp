#include "cobtesthelp.hpp"

#include "cobtmrlist.hpp"
#include "cobtmrwheel.hpp"
#include "cobtmrmill.hpp"
#include "cobtmrleanmill.hpp"


#include "perhaps.hpp"
#include "contingent.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <vector>
#include <cassert>

using namespace std::chrono_literals;
using ms = std::chrono::milliseconds;


template <class TMR> void basic(TMR& tmr)
{
    using namespace std::chrono_literals;
    using ID = typename TMR::tmrID;

    static_assert(std::is_convertible_v<int, ID>);

    tmr.start(0, 10ms);
    tmr.start(1, 20ms);
    tmr.start(2, 30ms);
    auto idx = tmr.start(3, 100ms);
    tmr.start(4, 250ms);

    assert(-1 == tmr.stop(333));
    assert(0 == tmr.stop(1));

    std::set<ID> expired;
    auto         count = [&expired](ID const& id) {
        if (id >= 0)
            expired.insert(id);
    };

    tmr.process_expired(5ms, count);
    assert(expired.empty());

    tmr.process_expired(10ms, count);
    assert(expired == std::set<int>{ 0 });
    expired.clear();

    tmr.process_expired(10ms, count);
    assert(expired.empty());

    tmr.process_expired(10ms, count);
    assert(expired == std::set<int>{ 2 });
    expired.clear();

    tmr.stop(idx);
    tmr.process_expired(100ms, count);
    assert(expired.empty());

    auto n = tmr.template stop_first<std::optional>();
    assert(n.has_value());
    assert(4 == *n);
    tmr.process_expired(100ms, count);
    assert(expired.empty());
}


struct nulify {
    bool operator()(int& id)
    {
        if (id != -1) {
            id = -1;
            return true;
        }
        else {
            return false;
        }
    }
};

void basic()
{
    cobtmrlist<int, 8, std::chrono::milliseconds> list;
    basic(list);

    cobtmrwheel<int, 8, std::chrono::milliseconds, 4> wheel;
    basic(wheel);

    cobtmrmill<int, 512, std::chrono::milliseconds, 3> mill;
    basic(mill);

    cobtmrleanmill<int, 512, std::chrono::milliseconds, nulify, 3> leanmill;
    basic(leanmill);
}


template <class U> struct ignore {
    ignore() {}
    ignore(U const&) {}
};

using list     = cobtmrlist<int, 8, ms>;
using wheel    = cobtmrwheel<int, 16, ms, 4>;
using mill     = cobtmrmill<int, 512, ms, 3>;
using leanmill = cobtmrleanmill<int, 512, ms, nulify, 3>;


// The ticks (counting from 1) at which timer `id` expired
template <class TMR> std::vector<int> expiries(TMR& tmr, int id, int ticks)
{
    std::vector<int> rslt;
    for (int t = 1; t <= ticks; ++t) {
        tmr.process_expired(1ms, [&](int x) {
            if (x == id) rslt.push_back(t);
        });
    }
    return rslt;
}


// A timer that can't be started must not disturb the ones running
void listfull()
{
    cobtmrlist<int, 2, ms> tmr;
    std::vector<int>       expired;
    auto                   note = [&](int id) { expired.push_back(id); };

    CHECK(tmr.empty());
    tmr.start(1, 10ms);
    tmr.start(2, 20ms);
    CHECK(!tmr.empty());
    tmr.start(3, 5ms); // no room
    tmr.process_expired(9ms, note);
    CHECK(expired.empty());
    tmr.process_expired(1ms, note);
    CHECK(expired == std::vector<int>{ 1 });
    tmr.process_expired(9ms, note);
    CHECK(expired == std::vector<int>{ 1 });
    tmr.process_expired(1ms, note);
    CHECK((expired == std::vector<int>{ 1, 2 }));
    CHECK(tmr.empty());
    CHECK(-1 == tmr.stop(1));
}


// Timers stopped, in any order, make room for new ones
void listrecycle()
{
    list tmr;
    for (int round = 0; round < 20; ++round) {
        list::index idx[8];
        for (int i = 0; i < 8; ++i) {
            idx[i] = tmr.start(i, ms{ 10 * (i + 1) });
        }
        for (int i = 0; i < 8; ++i) {
            const int at = (i * 3 + round) % 8;
            if (i % 2) {
                CHECK(0 == tmr.stop(at));
            }
            else {
                CHECK(0 == tmr.stop(idx[at]));
            }
        }
        CHECK(tmr.empty());
        CHECK(!tmr.template stop_first<std::optional>().has_value());
    }
}


void liststop()
{
    list             tmr;
    std::vector<int> expired;
    auto             note = [&](int id) { expired.push_back(id); };

    tmr.start(1, 10ms);
    tmr.start(2, 20ms);
    tmr.start(3, 20ms);
    tmr.start(4, 35ms);

    // those after the stopped one expire when they should
    CHECK(0 == tmr.stop(2));
    tmr.process_expired(19ms, note);
    CHECK(expired == std::vector<int>{ 1 });
    tmr.process_expired(1ms, note);
    CHECK((expired == std::vector<int>{ 1, 3 }));

    tmr.start(5, 5ms);
    auto first = tmr.template stop_first<std::optional>();
    CHECK(first.has_value() && (5 == *first));
    tmr.process_expired(14ms, note);
    CHECK((expired == std::vector<int>{ 1, 3 }));
    tmr.process_expired(1ms, note);
    CHECK((expired == std::vector<int>{ 1, 3, 4 }));

    tmr.start(6, 5ms);
    tmr.template stop_first<ignore>();
    CHECK(tmr.empty());
    tmr.template stop_first<ignore>();
    CHECK(-1 == tmr.stop(list::index{ cobic<8> }));

    tmr.start(7, 5ms);
    auto p = tmr.template stop_first<perhaps>();
    CHECK(p.has_value() && (7 == p.value_or(-1)));
    CHECK(!tmr.template stop_first<perhaps>().has_value());
}


// Random starts and stops: each timer must expire exactly when it should
template <class TMR> void exact(unsigned max_active)
{
    std::mt19937 rng(20231114);
    for (int round = 0; round < 20; ++round) {
        TMR                tmr;
        std::map<int, int> due; // ID -> tick
        int                next_id = 0;
        for (int now = 0; now < 1000; ++now) {
            for (unsigned i = rng() % 3; i > 0; --i) {
                if (due.size() < max_active) {
                    const int d = 1 + static_cast<int>(rng() % 60);
                    tmr.start(next_id, ms{ d });
                    due[next_id++] = now + d;
                }
            }
            if (!due.empty() && (rng() % 4 == 0)) {
                auto it = std::next(due.begin(), rng() % due.size());
                CHECK(0 == tmr.stop(it->first));
                due.erase(it);
            }
            tmr.process_expired(1ms, [&](int id) {
                auto it = due.find(id);
                CHECK(it != due.end());
                CHECK(it->second == now + 1);
                due.erase(it);
            });
            for (auto const& [id, tick] : due) {
                CHECK(tick > now + 1);
            }
        }
    }
}


void wheelbasics()
{
    // the same duration expires after the same time, wherever the wheel is
    for (int turned = 0; turned < 9; ++turned) {
        for (int d = 1; d < 20; ++d) {
            wheel tmr;
            tmr.process_expired(ms{ turned }, [](int) { CHECK(false); });
            tmr.start(1, ms{ d });
            CHECK(expiries(tmr, 1, 3 * d + 9) == std::vector<int>{ d });
        }
    }

    // no duration at all means: on the next tick
    wheel tmr;
    tmr.start(1, 0ms);
    CHECK(expiries(tmr, 1, 9) == std::vector<int>{ 1 });

    auto idx = tmr.start(2, 7ms);
    tmr.start(3, 3ms);
    CHECK(0 == tmr.stop(idx));
    CHECK(-1 == tmr.stop(2));
    CHECK(0 == tmr.stop(3));
    CHECK(-1 == tmr.stop(3));
    tmr.process_expired(20ms, [](int) { CHECK(false); });

    // any "optional" will do
    tmr.template stop_first<ignore>();
    CHECK(!tmr.template stop_first<std::optional>().has_value());
    CHECK(!tmr.template stop_first<perhaps>().has_value());
    tmr.start(4, 2ms);
    tmr.template stop_first<ignore>();
    CHECK(!tmr.template stop_first<std::optional>().has_value());
    tmr.start(5, 2ms);
    CHECK(5 == tmr.template stop_first<perhaps>().value_or(-1));
    tmr.start(6, 6ms);
    CHECK(6 == tmr.template stop_first<std::optional>());
    tmr.process_expired(20ms, [](int) { CHECK(false); });
}


// First level of a mill is precise: expires on the tick after the duration
template <class TMR> void millprecise()
{
    for (int turned = 0; turned < 130; turned += 13) {
        for (int d = 0; d < 64; d += 7) {
            TMR tmr;
            tmr.process_expired(ms{ turned }, [](int) { CHECK(false); });
            auto idx = tmr.start(1, ms{ d });
            CHECK(idx.valid());
            CHECK(expiries(tmr, 1, 200) == std::vector<int>{ d + 1 });
        }
    }
    // Further levels are not, yet should not be off by more than a turn
    for (int d = 64; d < 4000; d += 37) {
        TMR tmr;
        tmr.start(1, ms{ d });
        const auto when = expiries(tmr, 1, 5000);
        CHECK(when.size() == 1);
        CHECK(when[0] > d - 64 && when[0] <= d + 64);
    }

    static_assert(TMR::max_duration == ms{ 64 * 64 * 64 - 1 });
    TMR tmr;
    CHECK(tmr.start(1, TMR::max_duration).valid());
    CHECK(!tmr.start(2, TMR::max_duration + 1ms).valid());
    CHECK(!tmr.start(2, ms{ (1LL << 32) + 5 }).valid());
    CHECK(!tmr.start(2, ms{ -1 }).valid());
    CHECK(-1 == tmr.stop(2));
    CHECK(0 == tmr.stop(1));
}


// When a spoke is full, a timer is not started and its index is not valid
template <class TMR> void millfull()
{
    TMR                             tmr;
    std::vector<typename TMR::index> idx;
    for (int i = 0; i < 10; ++i) {
        idx.push_back(tmr.start(i, 5ms));
    }
    int started = 0;
    while ((started < 10) && idx[started].valid()) {
        ++started;
    }
    CHECK(started > 0 && started < 10);
    for (int i = started; i < 10; ++i) {
        CHECK(!idx[i].valid());
        tmr.stop(idx[i]); // does nothing
    }
    std::multiset<int> expired;
    tmr.process_expired(10ms, [&](int id) { expired.insert(id); });
    CHECK(static_cast<int>(expired.size()) == started);
    for (int i = 0; i < started; ++i) {
        CHECK(expired.count(i) == 1);
    }
}


// Stopping by index stops just the timer that was started
template <class TMR> void millstop()
{
    TMR  tmr;
    auto i1 = tmr.start(1, 5ms);
    auto i2 = tmr.start(2, 5ms);
    auto i3 = tmr.start(3, 5ms);
    CHECK(i1.valid() && i2.valid() && i3.valid());
    tmr.stop(i2);

    std::multiset<int> expired;
    auto               note = [&](int id) {
        if (id >= 0) expired.insert(id);
    };
    tmr.process_expired(10ms, note);
    CHECK((expired == std::multiset<int>{ 1, 3 }));

    // the first one that is still running
    expired.clear();
    i1 = tmr.start(1, 5ms);
    tmr.start(2, 5ms);
    tmr.start(3, 9ms);
    tmr.stop(i1);
    auto first = tmr.template stop_first<std::optional>();
    CHECK(first.has_value() && (2 == *first));
    first = tmr.template stop_first<std::optional>();
    CHECK(first.has_value() && (3 == *first));
    CHECK(!tmr.template stop_first<std::optional>().has_value());
    tmr.template stop_first<ignore>();
    CHECK(!tmr.template stop_first<perhaps>().has_value());
    tmr.process_expired(20ms, note);
    CHECK(expired.empty());
}


// Starting and stopping timers while their expiry is being processed:
// timer 1 is periodic and keeps timer 2 from ever expiring.
template <class TMR> void reentry(int period, int ticks, int expected)
{
    TMR tmr;
    int count = 0;
    int other = 0;
    tmr.start(1, ms{ period });
    tmr.start(2, ms{ period + 5 });
    tmr.start(3, ms{ period });
    for (int t = 0; t < ticks; ++t) {
        tmr.process_expired(1ms, [&](int id) {
            if (id == 1) {
                ++count;
                CHECK(0 == tmr.stop(2));
                tmr.start(1, ms{ period });
                tmr.start(2, ms{ period + 5 });
            }
            else if (id == 3) {
                ++other;
                tmr.start(3, ms{ period });
            }
            else {
                CHECK(id != 2);
            }
        });
    }
    CHECK(count == expected);
    CHECK(other == expected);
}


// Stopping by a stale index is harmless
void liststale()
{
    list             tmr;
    std::vector<int> expired;
    auto             note = [&](int id) { expired.push_back(id); };
    tmr.start(1, 10ms);
    auto i2 = tmr.start(2, 20ms);
    tmr.start(3, 30ms);
    CHECK(0 == tmr.stop(i2));
    CHECK(-1 == tmr.stop(i2));
    tmr.process_expired(40ms, note);
    CHECK((expired == std::vector<int>{ 1, 3 }));
}

template <class TMR> void millstale()
{
    TMR  tmr;
    tmr.start(1, 5ms);
    auto i2 = tmr.start(2, 5ms);
    tmr.start(3, 5ms);
    tmr.stop(i2);
    tmr.stop(i2);
    std::multiset<int> expired;
    tmr.process_expired(10ms, [&](int id) { if (id >= 0) expired.insert(id); });
    CHECK((expired == std::multiset<int>{ 1, 3 }));
}


int main()
{
    basic();
    liststale();
    millstale<mill>();
    millstale<leanmill>();

    listfull();
    listrecycle();
    liststop();
    exact<cobtmrlist<int, 32, ms>>(32);
    exact<cobtmrwheel<int, 128, ms, 4>>(32);
    exact<cobtmrwheel<int, 320, ms, 10>>(32);
    wheelbasics();
    millprecise<mill>();
    millprecise<leanmill>();
    millfull<mill>();
    millfull<leanmill>();
    millstop<mill>();
    millstop<leanmill>();
    {
        using nlmill = cobtmrleanmill<int, 512, ms, nulifier<int, nothing_is<-1>>, 3>;
        nlmill tmr;
        basic(tmr);
        millstop<nlmill>();
        millfull<nlmill>();
    }
    reentry<list>(10, 100, 10);
    reentry<wheel>(10, 100, 10);
    reentry<wheel>(8, 80, 10);
    reentry<mill>(10, 110, 10);
    reentry<leanmill>(10, 110, 10);

    return 0;
}
