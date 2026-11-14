#include "cobring.hpp"
#include "cobtesthelp.hpp"

#include <deque>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

void basics()
{
    cobring<int, 3> r;
    CHECK(r.empty() && !r.full() && r.max_size() == 3);
    CHECK(!r.pop_front<std::optional>().has_value());
    CHECK(!r.front<std::optional>().has_value() && !r.back<std::optional>().has_value());
    CHECK(!r.with_front([](int&) {}));
    CHECK(r.push_back(1) && r.push_back(2) && r.push_back(3));
    CHECK(r.full() && !r.push_back(4));
    CHECK(r.front<std::optional>() == 1 && r.back<std::optional>() == 3);
    CHECK(r.at<std::optional>(1) == 2 && !r.at<std::optional>(3).has_value());
    CHECK(r.pop_front<std::optional>() == 1);
    CHECK(r.push_back(4));
    std::vector<int> all;
    r.for_each([&](int& x) { all.push_back(x); });
    CHECK((all == std::vector<int>{ 2, 3, 4 }));
    CHECK(r.with_front([](int& x) { x = 20; }));
    CHECK(r.front<std::optional>() == 20);
    cobring<int, 3> const& cr = r;
    int                    sum = 0;
    cr.for_each([&](int const& x) { sum += x; });
    CHECK(sum == 27);
    r.clear();
    CHECK(r.empty());

    // wrap around many times
    for (int i = 0; i < 100; ++i) {
        CHECK(r.push_back(i));
        if (r.size() == 3) {
            CHECK(r.pop_front<std::optional>() == i - 2);
        }
    }

    // emplace_over keeps the newest N
    cobring<int, 3> o;
    for (int i = 0; i < 10; ++i) { o.emplace_over(i); }
    all.clear();
    o.for_each([&](int& x) { all.push_back(x); });
    CHECK((all == std::vector<int>{ 7, 8, 9 }));

    // N == 1
    cobring<int, 1> one;
    CHECK(one.push_back(1) && !one.push_back(2) && one.full());
    one.emplace_over(5);
    CHECK(one.size() == 1 && one.front<std::optional>() == 5 && one.back<std::optional>() == 5);
    CHECK(one.pop_front<std::optional>() == 5 && one.empty());
}

void objects()
{
    const int base = counted::alive;
    {
        cobring<counted, 4> r;
        for (int i = 0; i < 4; ++i) { r.emplace_back(i); }
        CHECK(counted::alive == base + 4);
        r.pop_front<std::optional>();
        CHECK(counted::alive == base + 3);
        r.emplace_back(7);
        r.emplace_over(8);
        CHECK(counted::alive == base + 4);
        cobring<counted, 4> c = r;
        CHECK(counted::alive == base + 8);
        std::vector<int> a, b;
        r.for_each([&](counted& x) { a.push_back(x.v); });
        c.for_each([&](counted& x) { b.push_back(x.v); });
        CHECK(a == b);
        cobring<counted, 4> d;
        d.emplace_back(1);
        d = r;
        CHECK(counted::alive == base + 12);
        auto& self = d;
        d          = self;
        CHECK(counted::alive == base + 12);
        c.clear();
        CHECK(counted::alive == base + 8);
    }
    CHECK(counted::alive == base);

    cobring<std::unique_ptr<int>, 2> u;
    CHECK(u.push_back(std::make_unique<int>(1)));
    CHECK(u.emplace_back(new int(2)));
    CHECK(!u.push_back(std::make_unique<int>(3)));
    auto p = u.pop_front<std::optional>();
    CHECK(p.has_value() && **p == 1);

    cobring<std::string, 2> s;
    s.push_back(std::string(50, 'a'));
    CHECK(s.front<std::optional>()->size() == 50);
}

void model()
{
    cobring<int, 7> r;
    std::deque<int> ref;
    std::mt19937    gen(2);
    for (int op = 0; op < 20000; ++op) {
        const int v = static_cast<int>(gen() % 1000);
        switch (gen() % 4) {
        case 0:
            CHECK(r.push_back(v) == (ref.size() < 7));
            if (ref.size() < 7) { ref.push_back(v); }
            break;
        case 1: {
            auto x = r.pop_front<std::optional>();
            CHECK(x.has_value() == !ref.empty());
            if (!ref.empty()) { CHECK(*x == ref.front()); ref.pop_front(); }
            break;
        }
        case 2:
            r.emplace_over(v);
            if (ref.size() == 7) { ref.pop_front(); }
            ref.push_back(v);
            break;
        default:
            break;
        }
        CHECK(r.size() == ref.size());
        CHECK(r.front<std::optional>().has_value() == !ref.empty());
        if (!ref.empty()) {
            CHECK(*r.front<std::optional>() == ref.front() && *r.back<std::optional>() == ref.back());
        }
        for (unsigned i = 0; i < 9; ++i) {
            auto x = r.at<std::optional>(i);
            CHECK(x.has_value() == (i < ref.size()));
            if (x) { CHECK(*x == ref[i]); }
        }
    }
}

int main()
{
    basics();
    objects();
    model();

    return 0;
}
