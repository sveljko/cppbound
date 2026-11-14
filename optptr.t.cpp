#include "optptr.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>


int main()
{
    int i = 3;
    int j = 5;

    optptr<int> some(&i);
    optptr<int> none(nullptr);
    optptr<int> null(static_cast<int*>(nullptr));
    CHECK(some);
    CHECK(!none);
    CHECK(!null);

    auto fi = make_fullptr<std::optional>(&i);
    auto fj = make_fullptr<std::optional>(&j);
    CHECK(fi.has_value() && fj.has_value());
    optptr<int> from(*fi);
    CHECK(from);

    CHECK(some.value_or(*fj).get() == &i);
    CHECK(none.value_or(*fj).get() == &j);
    optptr<int> const csome = some;
    optptr<int> const cnone = none;
    CHECK(csome.value_or(*fj).get() == &i);
    CHECK(cnone.value_or(*fj).get() == &j);
    static_assert(std::is_same_v<decltype(csome.value_or(*fj)), fullptr<int>>);

    // only called if there is something to point to
    int called = 0;
    some.and_then([&](int& p) { ++called; p += 1; });
    none.and_then([&](int&) { ++called; });
    csome.and_then([&](int const&) { ++called; });
    cnone.and_then([&](int const&) { ++called; });
    CHECK(called == 2);
    CHECK(i == 4);
    CHECK(some.and_then([](int& p) { return p * 2; }) == 8);
    CHECK(none.and_then([](int& p) { return p * 2; }) == 0);
    CHECK(csome.and_then([](int const& p) { return p * 2; }) == 8);
    CHECK(cnone.and_then([](int const& p) { return p * 2; }) == 0);
    CHECK(some.and_then([](int& p) { return std::to_string(p); }) == "4");
    CHECK(none.and_then([](int& p) { return std::to_string(p); }).empty());
    CHECK(some.and_then([](int& p) -> int& { return p; }) == 4);

    CHECK(some.or_else([&] { return optptr<int>(&j); }).value_or(*fj).get() == &i);
    CHECK(none.or_else([&] { return optptr<int>(&j); }).value_or(*fi).get() == &j);
    CHECK(cnone.or_else([&] { return optptr<int>(&j); }).value_or(*fi).get() == &j);
    CHECK(!none.or_else([] { return optptr<int>(nullptr); }));

    optptr<int> copy = some;
    copy             = none;
    CHECK(!copy);
    copy = some;
    CHECK(copy);
    CHECK(copy.release() == &i);
    CHECK(!copy);
    CHECK(copy.release() == nullptr);
    copy = from;
    copy.reset();
    CHECK(!copy);
    CHECK(from);

    static_assert(sizeof(optptr<int>) == sizeof(int*));
    static_assert(std::is_trivially_copyable_v<optptr<int>>);
    static_assert(std::is_trivially_destructible_v<optptr<int>>);
    static_assert(!std::is_convertible_v<optptr<int>, bool>);
    static_assert(!std::is_convertible_v<optptr<int>, int>);
    static_assert(std::is_constructible_v<bool, optptr<int>>);

    constexpr optptr<int> cnull(nullptr);
    static_assert(!cnull);

    return 0;
}
