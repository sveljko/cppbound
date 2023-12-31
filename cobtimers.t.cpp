#include "cobtesthelp.hpp"

#include "cobtmrlist.hpp"
#include "cobtmrwheel.hpp"
#include "cobtmrmill.hpp"
#include "cobtmrleanmill.hpp"


#include <chrono>
#include <optional>
#include <set>
#include <cassert>


template <class TMR> void basic(TMR& tmr)
{
    using namespace std::chrono_literals;
    using ID = typename TMR::tmrID;

    static_assert(std::is_convertible_v<int, ID>);

    tmr.start(0, 10ms);
    tmr.start(1, 20ms);
    tmr.start(2, 30ms);
    auto idx = tmr.start(3, 40ms);
    tmr.start(4, 50ms);

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
    tmr.process_expired(10ms, count);
    assert(expired.empty());

    auto n = tmr.template stop_first<std::optional>();
    assert(n.has_value());
    assert(4 == *n);
    tmr.process_expired(10ms, count);
    assert(expired.empty());
}


    struct nulify {
        void operator()(int& id) { id = -1; }
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


int main()
{
    basic();

    return 0;
}
