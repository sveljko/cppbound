#include "optunique.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<optunique<int>> && std::is_move_constructible_v<optunique<int>>);
static_assert(!std::is_copy_assignable_v<optunique<int>> && std::is_move_assignable_v<optunique<int>>);
static_assert(!std::is_convertible_v<optunique<int>, bool>);
static_assert(std::is_constructible_v<bool, optunique<int>>);


void opt()
{
    const int base = counted::alive;
    {
        optunique<counted> none = nullptr;
        CHECK(!none);
        optunique<counted> a = optunique<counted>::make(1);
        optunique<counted> b = std::make_unique<counted>(2);
        CHECK(a && b && counted::alive == base + 2);

        int called = 0;
        a.and_then([&](counted& c) { ++called; c.v = 10; });
        none.and_then([&](counted&) { ++called; });
        CHECK(called == 1);
        CHECK(a.and_then([](counted& c) { return c.v * 2; }) == 20);
        CHECK(none.and_then([](counted& c) { return c.v * 2; }) == 0);
        CHECK(none.and_then([](counted& c) { return std::to_string(c.v); }).empty());

        optunique<counted> const& ca = a;
        CHECK(ca.and_then([](counted const& c) { return c.v; }) == 10);
        ca.and_then([&](counted const&) { ++called; });
        CHECK(called == 2);

        auto m = std::move(a);
        CHECK(!a && m); // moved-from is null, which is fine
        CHECK(a.and_then([](counted& c) { return c.v; }) == 0);
        m = std::move(b);
        CHECK(counted::alive == base + 1); // the old object of m is gone
        CHECK(m.and_then([](counted& c) { return c.v; }) == 2);

        auto p = m.release();
        CHECK(!m && p && p->v == 2 && counted::alive == base + 1);
        optunique<counted> n = std::move(p);
        n.reset();
        CHECK(!n && counted::alive == base);
    }
    CHECK(counted::alive == base);
}

int main()
{
    opt();

    return 0;
}
