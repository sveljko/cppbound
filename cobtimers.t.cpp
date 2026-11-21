#include "cobtesthelp.hpp"

#include "atbtmrleanmill.hpp"
#include "atbtmrlist.hpp"
#include "atbtmrmill.hpp"
#include "atbtmrwheel.hpp"
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

    cobtmrmill<int, 512, std::chrono::milliseconds, 3, 64, true> exactmill;
    basic(exactmill);
}


template <class U> struct ignore {
    ignore() {}
    ignore(U const&) {}
};

using list     = cobtmrlist<int, 8, ms>;
using wheel    = cobtmrwheel<int, 16, ms, 4>;
using mill     = cobtmrmill<int, 512, ms, 3>;
using leanmill = cobtmrleanmill<int, 512, ms, nulify, 3>;
using exactmill = cobtmrmill<int, 512, ms, 3, 64, true>;


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


// The exact mill is precise at all levels: the tick after the duration,
// wherever the wheels are when the timer is started
// `small` is of 8 timers, 3 levels and 8 spokes, `exactmill` of 3 levels and 64 spokes
template <class small, class exactmill> void millexact()
{
    static_assert(small::max_duration == ms{ 8 * 8 * 8 - 1 });
    static_assert(exactmill::max_duration == mill::max_duration);

    for (int turned = 0; turned < 1100; turned += 7) {
        for (int d = 0; d < 512; d += ((d < 70) || (d > 500)) ? 1 : 13) {
            small tmr;
            tmr.process_expired(ms{ turned }, [](int) { CHECK(false); });
            CHECK(tmr.start(1, ms{ d }).valid());
            CHECK(expiries(tmr, 1, 1100) == std::vector<int>{ d + 1 });
        }
    }
    for (int d : { 0, 1, 63, 64, 65, 4095, 4096, 4097, 100000, 64 * 64 * 64 - 1 }) {
        exactmill tmr;
        tmr.process_expired(ms{ 4000 + d % 100 }, [](int) { CHECK(false); });
        CHECK(tmr.start(1, ms{ d }).valid());
        CHECK(expiries(tmr, 1, d + 70) == std::vector<int>{ d + 1 });
    }

    small tmr;
    CHECK(tmr.start(1, small::max_duration).valid());
    CHECK(!tmr.start(2, small::max_duration + 1ms).valid());
    CHECK(!tmr.start(2, ms{ (1LL << 32) + 5 }).valid());
    CHECK(!tmr.start(2, ms{ -1 }).valid());
    CHECK(-1 == tmr.stop(2));
    CHECK(0 == tmr.stop(1));
    CHECK(-1 == tmr.stop(1));

    // All the room is there for any spoke; the ones started first expire first
    std::vector<typename small::index> idx;
    for (int i = 0; i < 10; ++i) {
        idx.push_back(tmr.start(i, 100ms));
        CHECK(idx.back().valid() == (i < 8));
    }
    tmr.stop(idx[9]); // does nothing
    tmr.stop(idx[3]);
    tmr.stop(idx[3]); // neither does this
    CHECK(tmr.start(10, 100ms).valid() && !tmr.start(11, 100ms).valid());
    std::vector<int> expired;
    tmr.process_expired(100ms, [&](int id) { expired.push_back(id); });
    CHECK(expired.empty());
    tmr.process_expired(1ms, [&](int id) { expired.push_back(id); });
    CHECK((expired == std::vector<int>{ 0, 1, 2, 4, 5, 6, 7, 10 }));
    CHECK(!tmr.template stop_first<std::optional>().has_value());

    // The one that is first to expire, whatever the levels they are at
    tmr.start(1, 300ms);
    tmr.start(2, 70ms);
    tmr.start(3, 5ms);
    tmr.start(4, small::max_duration);
    tmr.process_expired(2ms, [](int) { CHECK(false); });
    tmr.start(5, 4ms);
    for (int id : { 3, 5, 2, 1, 4 }) {
        CHECK(id == tmr.template stop_first<std::optional>());
    }
    CHECK(!tmr.template stop_first<perhaps>().has_value());
    tmr.process_expired(600ms, [](int) { CHECK(false); });
}


// The index of a timer of the exact mill is checked: once the timer is not
// running, it is the index of nothing, whatever takes its place
template <class small> void millexactindex()
{
    small tmr;
    typename small::index none{};
    CHECK(!none.valid());
    tmr.stop(none);

    const auto a = tmr.start(1, 5ms);
    CHECK(a.valid());
    tmr.stop(a);
    const auto b = tmr.start(2, 5ms);       // in the slot that the timer 1 was in
    tmr.stop(a);                            // so this must not stop the timer 2
    CHECK(expiries(tmr, 2, 10) == std::vector<int>{ 6 });

    // the same when the timer has expired
    const auto c = tmr.start(3, 2ms);
    tmr.process_expired(5ms, [](int id) { CHECK(id == 3); });
    const auto d = tmr.start(4, 2ms);
    tmr.stop(c);
    tmr.stop(b);
    CHECK(expiries(tmr, 4, 10) == std::vector<int>{ 3 });
    tmr.stop(d);
    CHECK(!tmr.template stop_first<std::optional>().has_value());

    // over and over, in all the slots
    std::vector<typename small::index> old;
    for (int round = 0; round < 50; ++round) {
        std::vector<typename small::index> now;
        for (int i = 0; i < 8; ++i) {
            now.push_back(tmr.start(round * 8 + i, 3ms));
            CHECK(now.back().valid());
        }
        for (auto const& i : old) {
            tmr.stop(i);
        }
        int expired = 0;
        tmr.process_expired(4ms, [&](int id) {
            CHECK(id / 8 == round);
            ++expired;
        });
        CHECK(expired == 8);
        old = now;
    }
}


// Random starts and stops, with durations of all the levels and as many
// timers as there is room for: each must expire exactly when it should
// `small` is of 48 timers, 3 levels and 8 spokes
template <class small> void millexactrandom()
{
    std::mt19937 rng(20241116);
    for (int round = 0; round < 10; ++round) {
        small                       tmr;
        std::map<int, int>          due; // ID -> tick
        std::map<int, typename small::index> idx;
        int                         next_id = 0;
        for (int now = 0; now < 6000; ++now) {
            for (unsigned i = rng() % 4; i > 0; --i) {
                const int d  = static_cast<int>(rng() % 512);
                const auto at = tmr.start(next_id, ms{ d });
                CHECK(at.valid() == (due.size() < 48));
                if (at.valid()) {
                    idx[next_id] = at;
                    due[next_id] = now + d + 1;
                }
                ++next_id;
            }
            if (!due.empty() && (rng() % 4 == 0)) {
                auto it = std::next(due.begin(), rng() % due.size());
                if (rng() % 2) {
                    CHECK(0 == tmr.stop(it->first));
                }
                else {
                    tmr.stop(idx[it->first]);
                }
                CHECK(-1 == tmr.stop(it->first));
                due.erase(it);
            }
            if (!due.empty() && (rng() % 16 == 0)) {
                int soonest = due.begin()->second;
                for (auto const& [id, tick] : due) {
                    soonest = std::min(soonest, tick);
                }
                const auto first = tmr.template stop_first<std::optional>();
                CHECK(first.has_value() && (due.at(*first) == soonest));
                due.erase(*first);
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
template <class list> void liststale()
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


// With checked indices, the index of a timer that is not running any more is
// the index of nothing, whatever was started since. `turn` is the number of
// ticks after which a timer of the same duration gets to the same spoke.
template <class TMR> void checkedindex(int turn)
{
    TMR tmr;

    // stopped, and another one started in its place
    const auto a = tmr.start(1, 5ms);
    tmr.stop(a);
    tmr.start(2, 5ms);
    tmr.stop(a);        // must not stop the timer 2
    tmr.stop(a);
    CHECK(expiries(tmr, 2, 20).size() == 1);

    // expired, and another one started in its place
    const auto c       = tmr.start(3, 2ms);
    int        expired = 0;
    tmr.process_expired(ms{ turn }, [&](int id) { expired += (id == 3); });
    CHECK(expired == 1);
    const auto d = tmr.start(4, 2ms);
    tmr.stop(c);        // must not stop the timer 4
    CHECK(expiries(tmr, 4, 20).size() == 1);
    tmr.stop(d);        // it is not running either, by now

    // while it is running, its index does stop it
    const auto e = tmr.start(5, 3ms);
    tmr.stop(c);
    tmr.stop(a);
    tmr.stop(e);
    tmr.process_expired(ms{ 2 * turn }, [](int id) { CHECK(id != 5); });

    // over and over
    for (int round = 0; round < 3 * turn; ++round) {
        const auto old = tmr.start(100 + round, 1ms);
        tmr.process_expired(ms{ 1 + round % 3 }, [](int) {});
        tmr.process_expired(ms{ 2 }, [](int) {});
        const auto fresh = tmr.start(1000 + round, 4ms);
        tmr.stop(old);
        CHECK(expiries(tmr, 1000 + round, 10).size() == 1);
        tmr.stop(fresh);
    }
}

// The timers with checked indices are otherwise the same timers
void checked()
{
    using clist     = cobcheckedtmrlist<int, 8, ms>;
    using cwheel    = cobcheckedtmrwheel<int, 16, ms, 4>;
    using cmill     = cobcheckedtmrmill<int, 512, ms, 3>;
    using cleanmill = cobcheckedtmrleanmill<int, 512, ms, nulify, 3>;
    static_assert(std::is_same_v<cobcheckedtmrmill<int, 512, ms, 3, 64, true>, exactmill>);
    // not checked, the lean mill is as lean as it was
    static_assert(sizeof(leanmill) < sizeof(cleanmill));
    {
        clist     l;
        cwheel    w;
        cmill     m;
        cleanmill lm;
        basic(l);
        basic(w);
        basic(m);
        basic(lm);
    }
    liststale<clist>();
    millstale<cmill>();
    millstale<cleanmill>();
    exact<cobcheckedtmrlist<int, 32, ms>>(32);
    exact<cobcheckedtmrwheel<int, 128, ms, 4>>(32);
    millprecise<cmill>();
    millprecise<cleanmill>();
    millfull<cmill>();
    millfull<cleanmill>();
    millstop<cmill>();
    millstop<cleanmill>();
    reentry<clist>(10, 100, 10);
    reentry<cwheel>(10, 100, 10);
    reentry<cmill>(10, 110, 10);
    reentry<cleanmill>(10, 110, 10);

    checkedindex<clist>(8);
    checkedindex<cwheel>(4);
    checkedindex<cmill>(64);
    checkedindex<cleanmill>(64);
    checkedindex<exactmill>(64);
}

// The timers whose number is set at run time are the same code, so they
// have to pass the same tests
template <int N> struct timers {};      // the tag of the dimension N

void atb()
{
    for (bool set : { atbdim<timers<8>>::set(8), atbdim<timers<16>>::set(16), atbdim<timers<32>>::set(32),
                      atbdim<timers<48>>::set(48), atbdim<timers<128>>::set(128), atbdim<timers<320>>::set(320),
                      atbdim<timers<512>>::set(512) }) {
        CHECK(set);
    }
    using alist     = atbtmrlist<int, timers<8>, ms>;
    using awheel    = atbtmrwheel<int, timers<16>, ms, 4>;
    using amill     = atbtmrmill<int, timers<512>, ms, 3>;
    using aleanmill = atbtmrleanmill<int, timers<512>, ms, nulify, 3>;
    using aexact    = atbtmrmill<int, timers<512>, ms, 3, 64, true>;
    {
        alist     l;
        awheel    w;
        amill     m;
        aleanmill lm;
        aexact    x;
        basic(l);
        basic(w);
        basic(m);
        basic(lm);
        basic(x);
    }
    liststale<alist>();
    millstale<amill>();
    millstale<aleanmill>();
    millstale<aexact>();

    exact<atbtmrlist<int, timers<32>, ms>>(32);
    exact<atbtmrwheel<int, timers<128>, ms, 4>>(32);
    exact<atbtmrwheel<int, timers<320>, ms, 10>>(32);
    millprecise<amill>();
    millprecise<aleanmill>();
    millfull<amill>();
    millfull<aleanmill>();
    millstop<amill>();
    millstop<aleanmill>();
    millstop<aexact>();
    millexact<atbtmrmill<int, timers<8>, ms, 3, 8, true>, aexact>();
    millexactrandom<atbtmrmill<int, timers<48>, ms, 3, 8, true>>();
    millexactindex<atbtmrmill<int, timers<8>, ms, 3, 8, true>>();

    reentry<alist>(10, 100, 10);
    reentry<awheel>(10, 100, 10);
    reentry<awheel>(8, 80, 10);
    reentry<amill>(10, 110, 10);
    reentry<aleanmill>(10, 110, 10);
    reentry<aexact>(10, 110, 10);
    reentry<aexact>(100, 1010, 10);

    checkedindex<atbcheckedtmrlist<int, timers<8>, ms>>(8);
    checkedindex<atbcheckedtmrwheel<int, timers<16>, ms, 4>>(4);
    checkedindex<atbcheckedtmrmill<int, timers<512>, ms, 3>>(64);
    checkedindex<atbcheckedtmrleanmill<int, timers<512>, ms, nulify, 3>>(64);
    checkedindex<atbcheckedtmrmill<int, timers<512>, ms, 3, 64, true>>(64);
    {
        atbcheckedtmrmill<int, timers<512>, ms, 3>              m;
        atbcheckedtmrleanmill<int, timers<512>, ms, nulify, 3>  lm;
        atbcheckedtmrwheel<int, timers<16>, ms, 4>              w;
        basic(m);
        basic(lm);
        basic(w);
    }

    // With the dimension not set, or with no more timers than spokes, there is no room
    atbtmrlist<int, timers<0>, ms>                 l0;
    atbtmrwheel<int, timers<0>, ms, 4>             w0;
    atbtmrmill<int, timers<0>, ms, 3>              m0;
    atbtmrmill<int, timers<0>, ms, 3, 8, true>     x0;
    atbtmrleanmill<int, timers<0>, ms, nulify, 3>  lm0;
    atbtmrmill<int, timers<128>, ms, 3>            few;     // 3 * 64 spokes
    atbtmrleanmill<int, timers<128>, ms, nulify, 3> leanfew;
    CHECK(l0.start(1, 5ms) == l0.start(2, 5ms) && l0.empty() && (-1 == l0.stop(1)));
    CHECK(-1 == w0.stop(w0.start(1, 5ms)));
    CHECK(!m0.start(1, 5ms).valid() && !x0.start(1, 5ms).valid() && !lm0.start(1, 5ms).valid());
    CHECK(!few.start(1, 5ms).valid() && !leanfew.start(1, 5ms).valid());
    auto never = [](int) { CHECK(false); };
    l0.process_expired(100ms, never);
    w0.process_expired(100ms, never);
    m0.process_expired(100ms, never);
    x0.process_expired(100ms, never);
    lm0.process_expired(100ms, never);
    few.process_expired(100ms, never);
    CHECK(!x0.template stop_first<std::optional>().has_value() && !m0.template stop_first<std::optional>().has_value());
}

// The timer that `stop_first()` stops is one of those that would expire
// before all the others, whatever was started and however much time passed
template <class TMR> void firstisfirst(unsigned longest)
{
    std::mt19937 gen(5);
    int          found = 0;
    for (int round = 0; round < 200; ++round) {
        TMR tmr;
        int id = 0;
        for (int step = 0; step < 30; ++step) {
            if (gen() % 3) {
                tmr.start(id++, ms{ 1 + gen() % longest });
            }
            else {
                tmr.process_expired(ms{ 1 + gen() % 5 }, [](int) {});
            }
        }
        TMR           left_running = tmr;
        std::set<int> firsts;
        for (unsigned tick = 0; (tick < 2 * longest + 10) && firsts.empty(); ++tick) {
            left_running.process_expired(1ms, [&](int x) {
                if (x >= 0) firsts.insert(x);       // the lean mill reports "nothing" too
            });
        }
        const auto first = tmr.template stop_first<std::optional>();
        CHECK(first.has_value() == !firsts.empty());
        CHECK(!first || (firsts.count(*first) == 1));
        found += first.has_value();
    }
    CHECK(found > 100);
}

int main()
{
    basic();
    liststale<list>();
    millstale<mill>();
    millstale<leanmill>();
    millstale<exactmill>();

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
    millstop<exactmill>();
    millexact<cobtmrmill<int, 8, ms, 3, 8, true>, exactmill>();
    millexactrandom<cobtmrmill<int, 48, ms, 3, 8, true>>();
    millexactindex<cobtmrmill<int, 8, ms, 3, 8, true>>();
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
    reentry<exactmill>(10, 110, 10);
    reentry<exactmill>(100, 1010, 10);

    firstisfirst<cobtmrlist<int, 64, ms>>(100);
    firstisfirst<cobtmrwheel<int, 64, ms, 4>>(100);
    firstisfirst<cobtmrwheel<int, 100, ms, 10>>(100);
    firstisfirst<cobcheckedtmrwheel<int, 64, ms, 4>>(100);
    firstisfirst<cobtmrmill<int, 512, ms, 3, 8>>(400);
    firstisfirst<cobtmrleanmill<int, 512, ms, nulify, 3, 8>>(400);
    firstisfirst<cobtmrmill<int, 64, ms, 3, 8, true>>(400);

    checked();
    atb();

    return 0;
}
