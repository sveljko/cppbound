#include "atbfunction.hpp"
#include "cobfunction.hpp"

#include "coblist.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <memory>
#include <optional>
#include <string>

struct room {};     // the tag of the dimension: 32 bytes

// No more than the room and what tells what is in it
static_assert(sizeof(cobfunction<void(), 16>) <= 16 + 2 * alignof(std::max_align_t));
static_assert(std::is_constructible_v<cobfunction<int(int)>, int (*)(int)>);
// what can't be called like that is not taken
static_assert(!std::is_constructible_v<cobfunction<int(int)>, int (*)(std::string)>);
static_assert(!std::is_constructible_v<cobfunction<int(int)>, int>);
// nor what can't be copied
static_assert(!std::is_constructible_v<cobfunction<void()>, decltype([p = std::unique_ptr<int>()] {})>);
// with the room set at run time, nothing is known to fit
static_assert(!std::is_constructible_v<atbfunction<int(int), room>, int (*)(int)>);

int twice(int x)
{
    return 2 * x;
}
int thrice(int x) noexcept
{
    return 3 * x;
}
struct adder {
    int k;
    int add(int x) const { return x + k; }
    int operator()(int x) { return x + k++; }   // changes itself
};

void basics()
{
    cobfunction<int(int)> f;
    CHECK(!f && f.empty() && f.capacity() == 3 * sizeof(void*));
    // calling what holds nothing is not an error
    CHECK(f(3) == 0 && !f.call<std::optional>(3).has_value() && !f.call<perhaps>(3).has_value());

    f = twice;
    CHECK(f && !f.empty() && f(3) == 6 && f.call<std::optional>(4) == 8);
    f = &thrice;
    CHECK(f(3) == 9);

    int base = 10;
    f        = [&base](int x) { return x + base; };
    CHECK(f(1) == 11);
    base = 20;
    CHECK(f(1) == 21);

    // a copy holds a copy
    f = adder{ 100 };
    CHECK(f(1) == 101 && f(1) == 102);      // what it holds may change when called
    auto g = f;
    CHECK(g(1) == 103 && g(1) == 104 && f(1) == 103);
    cobfunction<int(int)> const c = f;
    CHECK(c(0) == 103 && c(0) == 104);       // as with std::function
    g = c;
    CHECK(g(0) == 105);
    auto const& self = g;
    g                = self;
    CHECK(g(0) == 106);

    f.clear();
    CHECK(!f && f(1) == 0);
    g = f;
    CHECK(!g);

    // a null pointer is nothing to call
    int (*none)(int) = nullptr;
    f                = none;
    CHECK(!f && f(5) == 0);
    int (adder::*nomember)(int) const = nullptr;
    cobfunction<int(adder const&, int)> m = nomember;
    CHECK(!m);
    m = &adder::add;
    CHECK(m && m(adder{ 5 }, 1) == 6);

    // no result
    cobfunction<void(int&)> v;
    int                     n = 0;
    v(n);
    CHECK(!v.call(n) && n == 0);
    v = [](int& x) { ++x; };
    v(n);
    CHECK(v.call(n) && n == 2);

    // arguments are passed on as they are
    cobfunction<std::size_t(std::string, std::unique_ptr<int>)> sink = [](std::string s, std::unique_ptr<int> p) {
        return s.size() + static_cast<std::size_t>(*p);
    };
    CHECK(sink("abc", std::make_unique<int>(4)) == 7);
    cobfunction<std::string(std::string const&)> echo = [](std::string const& s) { return s + s; };
    CHECK(echo("ab") == "abab" && echo.call<std::optional>("c") == "cc");
}

// What is held is copied and destroyed with the function, and only then
void lifetimes()
{
    auto shared = std::make_shared<int>(7);
    {
        cobfunction<int(), 16> f = [shared] { return *shared; };
        CHECK(shared.use_count() == 2 && f() == 7);
        auto g = f;
        CHECK(shared.use_count() == 3);
        f = g;
        CHECK(shared.use_count() == 3);
        g.clear();
        CHECK(shared.use_count() == 2);
        f = [] { return 1; };
        CHECK(shared.use_count() == 1 && f() == 1);
        g = [shared] { return *shared + 1; };
        CHECK(shared.use_count() == 2 && g() == 8);
    }
    CHECK(shared.use_count() == 1);
}

struct big {
    char pad[64] = {};
    int  operator()(int x) const { return x + pad[0]; }
};

// What may not fit goes through assign(), which tells
void assigning()
{
    cobfunction<int(int), 16> f = twice;
    CHECK(!f.assign(big{}) && f(2) == 4);       // as it was
    CHECK(f.assign(thrice) && f(2) == 6);
    int (*none)(int) = nullptr;
    CHECK(f.assign(none) && !f);

    atbfunction<int(int), room> a;
    CHECK(!a && a(1) == 0 && a.capacity() == 32);
    CHECK(a.assign(twice) && a(4) == 8);
    CHECK(!a.assign(big{}) && a(4) == 8);
    int k = 5;
    CHECK(a.assign([&k](int x) { return x * k; }) && a(3) == 15);
    auto b = a;
    k      = 6;
    CHECK(b(3) == 18 && b.call<std::optional>(1) == 6);
    a.clear();
    CHECK(!a && b);
    auto shared = std::make_shared<int>(1);
    {
        atbfunction<int(int), room> c;
        CHECK(c.assign([shared](int x) { return x + *shared; }) && shared.use_count() == 2 && c(1) == 2);
        auto d = c;
        CHECK(shared.use_count() == 3);
    }
    CHECK(shared.use_count() == 1);

    struct unset {};
    atbfunction<void(), unset> nothing;
    CHECK(!nothing.assign([] {}) && !nothing.call());
}

// The function that can only be moved holds also what can only be moved
static_assert(!std::is_copy_constructible_v<cobmovefunction<void()>>);
static_assert(!std::is_copy_assignable_v<cobmovefunction<void()>>);
static_assert(std::is_nothrow_move_constructible_v<cobmovefunction<void()>>);
static_assert(std::is_constructible_v<cobmovefunction<void()>, decltype([p = std::unique_ptr<int>()] {})>);
static_assert(!std::is_constructible_v<atbmovefunction<void(), room>, void (*)()>);

void moveonly()
{
    cobmovefunction<int(int)> f;
    CHECK(!f && f(1) == 0 && !f.call<std::optional>(1).has_value());

    auto owned = std::make_unique<int>(40);
    f          = [p = std::move(owned)](int x) { return *p + x; };
    CHECK(f && f(2) == 42 && f.call<std::optional>(3) == 43);

    // moving it leaves what it was moved from holding nothing
    cobmovefunction<int(int)> g = std::move(f);
    CHECK(g && g(2) == 42 && !f && f(2) == 0);
    f = std::move(g);
    CHECK(f(1) == 41 && !g);
    f = cobmovefunction<int(int)>{};
    CHECK(!f);

    // what can be copied is fine too, and is moved in if it can be
    f = twice;
    CHECK(f(4) == 8);
    auto shared = std::make_shared<int>(1);
    {
        cobmovefunction<int(), 16> h = [shared] { return *shared; };
        CHECK(shared.use_count() == 2 && h() == 1);
        cobmovefunction<int(), 16> k = std::move(h);
        CHECK(shared.use_count() == 2 && k() == 1 && !h);    // moved, not copied
        k = [] { return 5; };
        CHECK(shared.use_count() == 1 && k() == 5);
        auto fn = [shared] { return 2; };
        k       = std::move(fn);
        CHECK(shared.use_count() == 2);
        k.clear();
        CHECK(shared.use_count() == 1 && !k);
    }
    CHECK(shared.use_count() == 1);

    // what is held is destroyed once, wherever it ended up
    counted::alive = 0;
    {
        cobmovefunction<int(), 16> a = [c = counted{ 7 }] { return c.v; };
        CHECK(counted::alive == 1 && a() == 7);
        cobmovefunction<int(), 16> b = std::move(a);
        CHECK(counted::alive == 1 && b() == 7);
        a = std::move(b);
        CHECK(counted::alive == 1 && a() == 7);
    }
    CHECK(counted::alive == 0);

    CHECK(!f.assign(big{}) && f(4) == 8 && f.assign([q = std::make_unique<int>(3)](int x) { return *q * x; }) && f(5) == 15);

    atbmovefunction<int(int), room> a;
    CHECK(a.assign([q = std::make_unique<int>(3)](int x) { return *q + x; }) && a(1) == 4);
    atbmovefunction<int(int), room> b = std::move(a);
    CHECK(b(1) == 4 && !a && a(1) == 0 && !b.assign(big{}) && b(2) == 5);

    coblist<cobmovefunction<int()>, 4> jobs;
    jobs.emplace_back([p = std::make_unique<int>(9)] { return *p; });
    int sum = 0;
    jobs.for_each([&](auto& job) { sum += job(); });
    CHECK(sum == 9);
}

// Something to do later, kept in a container
void incontainer()
{
    coblist<cobfunction<void(std::string&)>, 4> todo;
    todo.push_back([](std::string& s) { s += "a"; });
    todo.emplace_back([](std::string& s) { s += "b"; });
    todo.push_back({});
    std::string done;
    todo.for_each([&](auto& f) { f(done); });
    CHECK(done == "ab");
}

int main()
{
    CHECK(atbdim<room>::set(32));
    basics();
    lifetimes();
    assigning();
    moveonly();
    incontainer();
    return 0;
}
