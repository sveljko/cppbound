#include "optshared.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>
#include <type_traits>

static_assert(!std::is_convertible_v<optshared<int>, bool>);
static_assert(std::is_constructible_v<bool, optshared<int>>);

struct X { int x = 3; };


void opt()
{
    const int base = counted::alive;
    {
        optshared<counted> none = nullptr;
        optshared<counted> from_null{ std::shared_ptr<counted>{} };
        CHECK(!none && !from_null);

        auto               f = fullshared<counted>::make(1);
        optshared<counted> a = f;
        optshared<counted> b = std::make_shared<counted>(2);
        CHECK(a && b);
        CHECK(counted::alive == base + 2);

        auto fa = a.full<std::optional>();
        CHECK(fa.has_value() && (*fa == f));
        CHECK(!none.full<std::optional>().has_value());
        CHECK(none.value_or(f) == f);
        CHECK(a.value_or(fullshared<counted>::make(3)) == f);

        int called = 0;
        a.and_then([&](counted& c) { ++called; c.v = 10; });
        none.and_then([&](counted&) { ++called; });
        CHECK(called == 1 && f->v == 10);
        CHECK(a.and_then([](counted& c) { return c.v * 2; }) == 20);
        CHECK(none.and_then([](counted& c) { return c.v * 2; }) == 0);
        CHECK(none.and_then([](counted& c) { return std::to_string(c.v); }).empty());

        CHECK(a.or_else([&] { return b; }).and_then([](counted& c) { return c.v; }) == 10);
        CHECK(none.or_else([&] { return b; }).and_then([](counted& c) { return c.v; }) == 2);

        CHECK(a.shared().get() == f.shared().get());
        auto c = a;
        c.reset();
        CHECK(!c && a);
        CHECK(counted::alive == base + 2);
        b.reset();
        CHECK(counted::alive == base + 1);
    }
    CHECK(counted::alive == base);
}

int main()
{
    opt();

    return 0;
}
