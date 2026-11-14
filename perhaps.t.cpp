#include "perhaps.hpp"
#include "cobtesthelp.hpp"
#include "cobi.hpp"
#include "coblist.hpp"

#include <iostream>
#include <memory>
#include <string>


template <class M> auto join(M&& m) {
    return m.and_then([](auto&& x) { return x; });
}

perhaps<int> operator+(const perhaps<int>& x, const perhaps<int>& y) {
    return x.and_then([&](int x) { return y.and_then([&](int y) { return perhaps<int>(x + y); }); });
}

struct mile {
    operator int() const { return x; }
    int x = 3;
};

void lifetime()
{
    {
        perhaps<counted> none;
        CHECK(!none.has_value());
        CHECK(counted::alive == 0);

        perhaps<counted> one{ 7 };
        CHECK(one.has_value());
        CHECK(counted::alive == 1);

        perhaps<counted> copy = one; // of a non-const object
        CHECK(copy.has_value() && counted::alive == 2 && counted::copies == 1);
        const perhaps<counted> ccopy = copy;
        perhaps<counted>       copy2 = ccopy;
        CHECK(counted::alive == 4 && counted::copies == 3 && counted::moves == 0);

        perhaps<counted> moved = std::move(copy);
        CHECK(moved.has_value() && counted::alive == 5 && counted::moves == 1);
        CHECK(moved.and_then([](counted const& c) { return c.v; }) == 7);

        perhaps<counted> nocopy = none;
        perhaps<counted> nomove = std::move(none);
        CHECK(!nocopy.has_value() && !nomove.has_value() && counted::alive == 5);

        nocopy = one;
        CHECK(nocopy.has_value() && counted::alive == 6);
        nocopy = nomove;
        CHECK(!nocopy.has_value() && counted::alive == 5);
        nomove = std::move(one);
        CHECK(nomove.has_value() && counted::alive == 6);
        nomove = perhaps<counted>{ 9 };
        CHECK(counted::alive == 6);
        CHECK(nomove.and_then([](counted const& c) { return c.v; }) == 9);
        auto& self = nomove;
        nomove     = self;
        CHECK(nomove.has_value() && counted::alive == 6);
        nomove = {};
        CHECK(!nomove.has_value() && counted::alive == 5);

        moved.reset();
        CHECK(!moved.has_value() && counted::alive == 4);
        moved.reset();
        CHECK(counted::alive == 4);
    }
    CHECK(counted::alive == 0);

    {
        perhaps<std::string> s{ 3u, 'x' }; // several arguments
        CHECK(s.value_or("") == "xxx");
        perhaps<std::unique_ptr<int>> u{ std::make_unique<int>(5) }; // move-only
        CHECK(u.has_value());
        perhaps<std::unique_ptr<int>> w = std::move(u);
        CHECK(std::move(w).and_then([](std::unique_ptr<int>&& p) { return *p; }) == 5);
    }
    static_assert(!std::is_constructible_v<perhaps<int>, std::string>);
    static_assert(std::is_constructible_v<perhaps<std::string>, char const*>);
    static_assert(std::is_copy_constructible_v<perhaps<std::string>>);
    static_assert(!std::is_copy_constructible_v<perhaps<std::unique_ptr<int>>>);
}


void monadic()
{
    using namespace std::string_literals;

    perhaps<std::string>       some{ "abc" };
    const perhaps<std::string> none;

    int called = 0;
    some.and_then([&](std::string& s) { ++called; s += "d"; });
    none.and_then([&](std::string const&) { ++called; });
    CHECK(called == 1);
    CHECK(some.and_then([](std::string const& s) { return s.size(); }) == 4);
    CHECK(none.and_then([](std::string const& s) { return s.size(); }) == 0);
    CHECK(std::as_const(some).and_then([](std::string const& s) { return s; }) == "abcd");

    auto stolen = std::move(some).and_then([](std::string&& s) { return perhaps<std::string>(std::move(s)); });
    CHECK(stolen.value_or("?") == "abcd");
    CHECK(perhaps<std::string>{}.and_then([](std::string&& s) { return perhaps<std::string>(std::move(s)); }).value_or("?")
          == "?");

    CHECK(stolen.or_else([] { return perhaps<std::string>{ "else" }; }).value_or("?") == "abcd");
    CHECK(none.or_else([] { return perhaps<std::string>{ "else" }; }).value_or("?") == "else");
    CHECK(perhaps<std::string>{}.or_else([] { return perhaps<std::string>{ "else" }; }).value_or("?") == "else");
    CHECK(perhaps<std::string>{ "tmp" }.or_else([] { return perhaps<std::string>{ "else" }; }).value_or("?") == "tmp");

    auto len = stolen.transform([](std::string const& s) { return s.size(); });
    CHECK(len.value_or(0) == 4);
    CHECK(!none.transform([](std::string const& s) { return s.size(); }).has_value());
    // a reference returned is kept as a value
    auto same = stolen.transform([](std::string const& s) -> std::string const& { return s; });
    static_assert(std::is_same_v<decltype(same), perhaps<std::string>>);
    CHECK(same.value_or("?") == "abcd");

    CHECK(none.value_or("dflt") == "dflt");
    CHECK(none.value_or("dflt"s) == "dflt");
    CHECK(perhaps<std::string>{ "tmp" }.value_or("dflt") == "tmp");
    CHECK(perhaps<int>::unit(5).value_or(0) == 5);
    static_assert(std::is_same_v<decltype(perhaps<int>::unit("x"s)), perhaps<std::string>>);
}


// As the "optional" of our containers
void asoptional()
{
    using id = cobint<0, 4>;
    CHECK(id::be<perhaps>(2).and_then([](id i) { return i.get(); }) == 2);
    CHECK(!id::be<perhaps>(5).has_value());

    coblist<int, 3> l;
    CHECK(!l.pop_front<perhaps>().has_value());
    CHECK(l.push_back(8));
    CHECK(l.push_back(9));
    perhaps<int> x = l.pop_back<perhaps>();
    CHECK(x.value_or(0) == 9);
    x = l.pop_front<perhaps>();
    CHECK(x.value_or(0) == 8);
    x = l.pop_front<perhaps>();
    CHECK(!x.has_value());
}


void emplaceeq()
{
    perhaps<std::string> s;
    s.emplace(3u, 'a');
    CHECK(s.value_or("") == "aaa");
    s.emplace("zz");
    CHECK(s.value_or("") == "zz");
    perhaps<counted> c{ 1 };
    const int        alive = counted::alive;
    c.emplace(2);
    CHECK(counted::alive == alive);
    CHECK(c.and_then([](counted const& x) { return x.v; }) == 2);

    perhaps<int> a, b, x{ 1 }, y{ 1 }, z{ 2 };
    CHECK(a == b);
    CHECK(x == y);
    CHECK(!(x == z));
    CHECK(!(a == x));
    CHECK(x != z);
    CHECK(a != x);
}


int main() {
    emplaceeq();
    using namespace std::string_literals;

    lifetime();
    monadic();
    asoptional();

    perhaps<int> x = 10;
    perhaps<int> y = 20;

    x.transform([](int x) { return (x == 10) ? "success"s : "fail"s; })
    .and_then([](std::string const& r) { std::cout << r << std::endl; });

    (x + y).and_then([](int r) { std::cout << r << std::endl; });

    std::cout << join(x + x + y) << std::endl;

    perhaps<mile> z;
    mile q;
    z.or_else([] {return perhaps<mile>{};}).value_or(q);
    std::cout << z.value_or(mile{5}) << std::endl;
}
