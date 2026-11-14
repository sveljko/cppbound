#include "fullunique.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>
#include <type_traits>

static_assert(!std::is_default_constructible_v<fullunique<int>>);
static_assert(!std::is_move_constructible_v<fullunique<int>> && !std::is_copy_constructible_v<fullunique<int>>);


void full()
{
    const int base = counted::alive;
    {
        auto a = fullunique<counted>::make(5);
        CHECK(counted::alive == base + 1);
        CHECK(a->v == 5 && (*a).v == 5);
        a->v = 6;
        auto const& ca = a;
        CHECK(ca->v == 6 && (*ca).v == 6);
        static_assert(std::is_same_v<decltype(*ca), counted const&>);
    }
    CHECK(counted::alive == base);
}

int main()
{
    full();

    return 0;
}
