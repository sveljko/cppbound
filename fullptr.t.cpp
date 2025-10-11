#include "fullptr.hpp"

#include <optional>

#include <cassert>


int main()
{
    int i = 3;
    auto x = make_fullptr<std::optional>(&i);
    assert(x.has_value());
    assert(**x == i);

    std::optional<int> oi;
    assert(!oi.has_value());

    int *pi = nullptr;
    auto y = make_fullptr(pi);
    assert(!y.has_value());

    return 0;
}
