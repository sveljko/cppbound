#include "cobbinheap.hpp"
#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <vector>

void basics()
{
    cobbinheap<int, 6> h;
    CHECK(h.empty() && !h.full() && h.size() == 0 && h.max_size() == 6);
    CHECK(!h.top<std::optional>().has_value() && !h.pop<std::optional>().has_value());
    CHECK(!h.with_top([](int const&) { CHECK(false); }));

    for (int x : { 3, 9, 1, 7, 9, 4 }) {
        CHECK(h.push(x));
    }
    CHECK(h.full() && !h.push(100) && !h.emplace(100) && h.size() == 6);
    CHECK(h.top<std::optional>() == 9 && h.top<perhaps>().value_or(0) == 9);
    int top = 0;
    CHECK(h.with_top([&](int const& x) { top = x; }) && top == 9);
    int sum = 0;
    h.for_each([&](int const& x) { sum += x; });
    CHECK(sum == 33);

    std::vector<int> out;
    while (auto x = h.pop<std::optional>()) {
        out.push_back(*x);
    }
    CHECK((out == std::vector<int>{ 9, 9, 7, 4, 3, 1 }) && h.empty());

    CHECK(h.push(1) && h.push(2));
    h.clear();
    CHECK(h.empty() && h.push(5) && h.top<std::optional>() == 5);
}

struct later {
    bool operator()(std::unique_ptr<int> const& x, std::unique_ptr<int> const& y) const { return *x > *y; }
};

void anytype()
{
    // the smallest at the top
    cobbinheap<std::string, 4, std::greater<std::string>> s;
    CHECK(s.push("pear") && s.emplace(3u, 'a') && s.push("fig"));
    CHECK(s.pop<std::optional>() == "aaa" && s.pop<std::optional>() == "fig" && s.pop<std::optional>() == "pear");

    cobbinheap<std::unique_ptr<int>, 4, later> up;
    CHECK(up.emplace(new int(5)) && up.push(std::make_unique<int>(2)) && up.emplace(new int(9)));
    int top = 0;
    CHECK(up.with_top([&](std::unique_ptr<int> const& p) { top = *p; }) && top == 2);
    auto first = up.pop<std::optional>();
    CHECK(first.has_value() && **first == 2 && up.size() == 2);

    counted::alive = 0;
    {
        cobbinheap<counted, 8, decltype([](counted const& x, counted const& y) { return x.v < y.v; })> h;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 8; ++i) {
            CHECK(h.emplace(i * 5 % 8));
        }
        CHECK(counted::alive == 8 && !h.emplace(1) && counted::alive == 8);
        CHECK(h.pop<perhaps>().value_or(counted{ -1 }).v == 7);
        CHECK(counted::alive == 7);
        auto c = h;
        CHECK(counted::alive == 14 && c.size() == 7);
        c.clear();
        CHECK(counted::alive == 7 && h.size() == 7);
    }
    CHECK(counted::alive == 0);
}

void against_std()
{
    std::mt19937 rng(20241118);
    for (int round = 0; round < 20; ++round) {
        cobbinheap<int, 33>      h;
        std::priority_queue<int> model;
        for (int step = 0; step < 4000; ++step) {
            if (rng() % 5 < 3) {
                const int  x  = static_cast<int>(rng() % 50);
                const bool ok = h.push(x);
                CHECK(ok == (model.size() < 33));
                if (ok) {
                    model.push(x);
                }
            }
            else {
                const auto x = h.pop<std::optional>();
                CHECK(x.has_value() == !model.empty());
                if (x) {
                    CHECK(*x == model.top());
                    model.pop();
                }
            }
            CHECK(h.size() == model.size());
            CHECK(h.top<std::optional>().has_value() == !model.empty());
            if (!model.empty()) {
                CHECK(h.top<std::optional>() == model.top());
            }
        }
    }
}

int main()
{
    basics();
    anytype();
    against_std();
    return 0;
}
