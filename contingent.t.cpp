#include "contingent.hpp"
#include "coblist.hpp"
#include "cobtmrleanmill.hpp"
#include "cobtesthelp.hpp"

#include <climits>
#include <cmath>
#include <optional>
#include <chrono>
#include <set>
#include <vector>

using namespace std::chrono_literals;
using ms = std::chrono::milliseconds;

enum class color { red, green, none };

static_assert(sizeof(contingent<double>) == sizeof(double));
static_assert(sizeof(contingent<float>) == sizeof(float));
static_assert(sizeof(contingent<int*>) == sizeof(int*));
static_assert((sizeof(contingent<int, nothing_is<-1>>) == sizeof(int)));
static_assert(sizeof(contingent<cobint<0, 5>>) == sizeof(int));

static_assert(!contingent<double>{}.has_value());
static_assert(contingent<double>{ 1.5 }.value_or(0) == 1.5);
static_assert(!contingent<int, nothing_is<-1>>{ -1 }.has_value());

// Runs the same checks for each policy: `a` and `b` are two different values
template <class C, class T> void common(T a, T b)
{
    C none;
    CHECK(!none.has_value());
    C one(a);
    CHECK(one.has_value());

    int called = 0;
    one.and_then([&](auto const&) { ++called; });
    none.and_then([&](auto const&) { ++called; });
    CHECK(called == 1);
    CHECK(one.and_then([&](auto const&) { return 7; }) == 7);
    CHECK(none.and_then([&](auto const&) { return 7; }) == 0);

    CHECK(one.or_else([&] { return C(b); }) == one);
    CHECK(none.or_else([&] { return C(b); }) == C(b));

    auto t = one.transform([&](auto const&) { return 3; });
    CHECK(t.has_value() && (t.value_or(0) == 3));
    CHECK(!none.transform([&](auto const&) { return 3; }).has_value());

    CHECK(one.value_or(b) == a);
    CHECK(none.value_or(b) == b);

    CHECK(one == C(a));
    CHECK(!(one == C(b)));
    CHECK(!(one == none));
    CHECK(none == C{});
    CHECK(one != none);

    one.reset();
    CHECK(!one.has_value());
    CHECK(one == none);
}

void policies()
{
    int x = 1, y = 2;
    common<contingent<int*>>(&x, &y);
    common<contingent<double>>(1.5, -2.5);
    common<contingent<float>>(1.5f, -2.5f);
    common<contingent<int, nothing_is<-1>>>(3, 4);
    common<contingent<color, nothing_is<color::none>>>(color::red, color::green);
    common<contingent<cobint<0, 5>>>(cobint<0, 5>::smallest(), cobint<0, 5>::greatest());
    common<contingent<cobint<0, INT_MAX>>>(cobint<0, INT_MAX>::smallest(),
                                           cobint<0, INT_MAX>::greatest());

    // the sentinel itself is nothing
    CHECK(!contingent<int*>(nullptr).has_value());
    CHECK((!contingent<int, nothing_is<-1>>(-1).has_value()));
#if !defined(__FINITE_MATH_ONLY__) || !__FINITE_MATH_ONLY__
    CHECK(!contingent<double>(std::nan("")).has_value());
#endif

    // special values are values
#if !defined(__FINITE_MATH_ONLY__) || !__FINITE_MATH_ONLY__
    CHECK(contingent<double>(INFINITY).has_value());
    CHECK(contingent<double>(-INFINITY).has_value());
    CHECK(contingent<float>(INFINITY).has_value());
#endif
    CHECK(contingent<double>(std::numeric_limits<double>::max()).has_value());
    CHECK(contingent<double>(std::numeric_limits<double>::lowest()).has_value());
    CHECK(contingent<double>(std::numeric_limits<double>::denorm_min()).has_value());
    CHECK(contingent<double>(0.0).has_value());
    CHECK(contingent<float>(std::numeric_limits<float>::max()).has_value());

    // two nothings are equal, though two NaNs are not
    CHECK(contingent<double>{} == contingent<double>{});
    CHECK(contingent<float>{} == contingent<float>{});

    // the value of a range bound integer
    contingent<cobint<0, 5>> c(cobint<0, 5>::greatest());
    CHECK(c.value_or(cobint<0, 5>{}).get() == 5);
    contingent<cobint<0, INT_MAX>> d(cobint<0, INT_MAX>::greatest());
    CHECK(d.has_value() && (d.value_or(cobint<0, INT_MAX>{}).get() == INT_MAX));
}

void ascontainerresult()
{
    coblist<int*, 3> l;
    int              a = 1;
    l.push_back(&a);
    auto p = l.pop_front<contingently>();
    CHECK(p.has_value() && (p.value_or(nullptr) == &a));
    CHECK(!l.pop_front<contingently>().has_value());

    auto some = cobint<0, 4>::be<contingently>(2);
    CHECK(some.has_value() && (some.value_or(cobint<0, 4>{}).get() == 2));
    CHECK((!cobint<0, 4>::be<contingently>(5).has_value()));
}

template <class TMR> void timers()
{
    {
        TMR tmr;
        auto i1 = tmr.start(1, 5ms);
        auto i2 = tmr.start(2, 5ms);
        tmr.start(3, 5ms);
        CHECK(i1.valid() && i2.valid());
        tmr.stop(i2);
        std::multiset<int> expired;
        tmr.process_expired(10ms, [&](int id) { if (id >= 0) expired.insert(id); });
        CHECK((expired == std::multiset<int>{ 1, 3 }));
    }
    {
        TMR  tmr;
        auto idx = tmr.start(7, 5ms);
        CHECK(idx.valid());
        auto first = tmr.template stop_first<std::optional>();
        CHECK(first.has_value() && (7 == *first));
    }
}

int main()
{
    policies();
    ascontainerresult();
    timers<cobtmrleanmill<int, 512, ms, nulifier<int, nothing_is<-1>>, 3>>();

    return 0;
}
