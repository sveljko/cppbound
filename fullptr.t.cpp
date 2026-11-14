#include "fullptr.hpp"
#include "cobtesthelp.hpp"

#include <optional>
#include <string>

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

    CHECK(x->get() == &i);
    **x = 4;
    CHECK(i == 4);

    // copies point to the same object
    fullptr<int> p = *x;
    fullptr<int> const cp = p;
    CHECK(p.get() == &i && cp.get() == &i);
    CHECK(*cp == 4);
    static_assert(std::is_same_v<decltype(*cp), int const&>);
    static_assert(std::is_same_v<decltype(cp.get()), int const*>);

    int  j = 7;
    auto q = make_fullptr<std::optional>(&j);
    CHECK(q.has_value());
    p = *q;
    CHECK(*p == 7);
    *p = 8;
    CHECK(j == 8 && i == 4);

    std::string s  = "abc";
    auto        ps = make_fullptr(&s);
    CHECK(ps.has_value());
    CHECK(ps.and_then([](fullptr<std::string> f) { return f->size(); }) == 3);
    ps.and_then([](fullptr<std::string> f) { *f += "d"; });
    CHECK(s == "abcd");

    std::string const cs  = "const";
    auto              pcs = make_fullptr<std::optional>(&cs);
    CHECK(pcs.has_value() && (*pcs)->size() == 5);

    // there is no way to make one out of thin air
    static_assert(!std::is_default_constructible_v<fullptr<int>>);
    static_assert(!std::is_constructible_v<fullptr<int>, int*>);
    static_assert(!std::is_constructible_v<fullptr<int>, std::nullptr_t>);
    static_assert(!std::is_convertible_v<int*, fullptr<int>>);
    static_assert(std::is_copy_constructible_v<fullptr<int>>);
    static_assert(std::is_copy_assignable_v<fullptr<int>>);
    static_assert(std::is_trivially_copyable_v<fullptr<int>>);
    static_assert(sizeof(fullptr<int>) == sizeof(int*));

    // from a reference, and conversions
    int  k  = 6;
    auto fk = fullptr<int>::from(k);
    CHECK(fk.get() == &k);
    fullptr<int const> cfk = fk;
    CHECK(cfk.get() == &k);
    struct base { int b = 1; };
    struct derived : base { int d = 2; };
    derived dv;
    auto    fd = fullptr<derived>::from(dv);
    fullptr<base> fb = fd;
    CHECK(fb->b == 1 && fb.get() == static_cast<base*>(&dv));
    static_assert(!std::is_convertible_v<fullptr<int>, fullptr<long>>);
    static_assert(!std::is_convertible_v<fullptr<int const>, fullptr<int>>);
    CHECK(fk == fullptr<int>::from(k));
    int k2 = 6;
    CHECK(!(fk == fullptr<int>::from(k2)));

    constexpr auto none = make_fullptr<std::optional>(static_cast<int*>(nullptr));
    static_assert(!none.has_value());

    return 0;
}
