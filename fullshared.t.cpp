#include "fullshared.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>
#include <type_traits>

static_assert(!std::is_default_constructible_v<fullshared<int>>);
static_assert(!std::is_constructible_v<fullshared<int>, std::shared_ptr<int>>);

struct X { int x = 3; };


void full()
{
    const int base = counted::alive;
    {
        auto a = fullshared<counted>::make(5);
        CHECK(counted::alive == base + 1);
        CHECK(a->v == 5 && (*a).v == 5);
        auto b = a;
        CHECK(counted::alive == base + 1 && a == b);
        auto c = std::move(a); // copies: a is still usable
        CHECK(a->v == 5 && c->v == 5 && counted::alive == base + 1);
        b->v = 6;
        CHECK(a->v == 6 && c->v == 6);
        CHECK(a.shared().use_count() >= 3);

        auto d = fullshared<counted>::make(7);
        CHECK(!(d == a));
        d = a;
        CHECK(counted::alive == base + 1 && d == a);

        auto none = fullshared<counted>::from<std::optional>(nullptr);
        CHECK(!none.has_value());
        auto some = fullshared<counted>::from<std::optional>(std::make_shared<counted>(9));
        CHECK(some.has_value() && (*some)->v == 9);
        CHECK(counted::alive == base + 2);
    }
    CHECK(counted::alive == base);
}

int main()
{
    full();

    return 0;
}
