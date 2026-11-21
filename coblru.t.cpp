#include "atblru.hpp"
#include "coblru.hpp"

#include "cobtesthelp.hpp"
#include "perhaps.hpp"

#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

struct sized {};    // the tag of the dimension, which is 24

template <class C> std::string order(C const& c)
{
    std::string r;
    c.for_each([&](std::string const& k, int const&) { r += k; });
    return r;
}

template <class C> void basics()
{
    C c;
    CHECK(c.empty() && !c.full() && c.size() == 0 && c.max_size() == 3);
    CHECK(!c.template get<std::optional>("a").has_value() && !c.contains("a") && !c.erase("a"));
    CHECK(!c.template eldest<std::optional>().has_value());

    CHECK(c.put("a", 1) && c.put("b", 2) && c.put("c", 3) && c.full() && order(c) == "cba");
    CHECK(c.template eldest<std::optional>() == "a");

    // getting, changing and putting again are a use; peeking and looking are not
    CHECK(c.template get<std::optional>("a") == 1 && order(c) == "acb");
    CHECK(c.template peek<std::optional>("b") == 2 && c.contains("b") && order(c) == "acb");
    CHECK(c.with("b", [](int& v) { v = 20; }) && order(c) == "bac" && c.template peek<perhaps>("b").value_or(0) == 20);
    CHECK(!c.with("z", [](int&) { CHECK(false); }));
    CHECK(c.put("c", 30) && order(c) == "cba" && c.size() == 3 && c.template peek<std::optional>("c") == 30);
    CHECK(c.put("c", 31) && order(c) == "cba");     // the newest one, used again

    // no room: the one not used for the longest time goes
    std::vector<std::pair<std::string, int>> gone;
    auto note = [&](std::string const& k, int& v) { gone.push_back({ k, v }); };
    CHECK(c.put("d", 4, note) && order(c) == "dcb" && !c.contains("a") && c.size() == 3);
    CHECK((gone == std::vector<std::pair<std::string, int>>{ { "a", 1 } }));
    CHECK(c.put("b", 21, note) && gone.size() == 1);        // it is in: nothing goes
    CHECK(c.put("e", 5) && order(c) == "ebd" && !c.contains("c"));

    CHECK(c.erase("b") && !c.erase("b") && order(c) == "ed" && !c.full());
    CHECK(c.erase("e") && c.erase("d") && c.empty() && !c.template eldest<std::optional>().has_value());
    CHECK(c.put("x", 1) && c.put("y", 2));

    // a copy is a cache of its own
    C copy = c;
    CHECK(order(copy) == "yx" && copy.template get<std::optional>("x") == 1 && order(copy) == "xy" && order(c) == "yx");
    c.clear();
    CHECK(c.empty() && order(c).empty() && copy.size() == 2 && c.put("z", 9) && order(c) == "z");
    c.for_each([](std::string const&, int& v) { ++v; });
    CHECK(c.template peek<std::optional>("z") == 10);
}

// A model: a list of keys and values, the most recently used one first
template <class C> void against_model(unsigned capacity, int keys)
{
    std::mt19937 rng(20241122);
    for (int round = 0; round < 10; ++round) {
        C                              c;
        std::list<std::pair<int, int>> model;
        auto                           at = [&](int k) {
            auto it = model.begin();
            while ((it != model.end()) && (it->first != k)) {
                ++it;
            }
            return it;
        };
        for (int step = 0; step < 5000; ++step) {
            const int k  = static_cast<int>(rng() % keys) * 31;    // some of them hash alike
            const int v  = static_cast<int>(rng() % 1000);
            auto      it = at(k);
            switch (rng() % 6) {
            case 0:
            case 1: {
                int evicted = -1;
                CHECK(c.put(k, v, [&](int const& key, int&) { evicted = key; }));
                if (it != model.end()) {
                    model.erase(it);
                    CHECK(evicted == -1);
                }
                else if (model.size() == capacity) {
                    CHECK(evicted == model.back().first);
                    model.pop_back();
                }
                else {
                    CHECK(evicted == -1);
                }
                model.push_front({ k, v });
                break;
            }
            case 2: {
                const auto got = c.template get<std::optional>(k);
                CHECK(got.has_value() == (it != model.end()));
                if (got) {
                    CHECK(*got == it->second);
                    model.splice(model.begin(), model, it);
                }
                break;
            }
            case 3: {
                const auto got = c.template peek<std::optional>(k);
                CHECK(got.has_value() == (it != model.end()) && (!got || (*got == it->second)));
                break;
            }
            case 4:
                CHECK(c.erase(k) == (it != model.end()));
                if (it != model.end()) {
                    model.erase(it);
                }
                break;
            default:
                CHECK(c.with(k, [&](int& x) { x = v; }) == (it != model.end()));
                if (it != model.end()) {
                    it->second = v;
                    model.splice(model.begin(), model, it);
                }
            }
            CHECK(c.size() == model.size() && c.full() == (model.size() == capacity));
            CHECK(c.template eldest<std::optional>().has_value() == !model.empty());
            if (!model.empty()) {
                CHECK(c.template eldest<std::optional>() == model.back().first);
            }
            auto mit = model.begin();
            c.for_each([&](int const& key, int const& value) {
                CHECK(mit != model.end() && mit->first == key && mit->second == value);
                ++mit;
            });
            CHECK(mit == model.end());
        }
    }
}

struct alike {
    std::size_t operator()(int) const { return 3; }    // all the keys in one chain
};

void anytype()
{
    counted::alive = 0;
    {
        coblru<int, counted, 4> c;
        CHECK(counted::alive == 0);
        for (int i = 0; i < 10; ++i) {
            CHECK(c.put(i, counted{ i }));
        }
        CHECK(counted::alive == 4 && c.size() == 4);
        CHECK(c.erase(9) && counted::alive == 3);
        auto copy = c;
        CHECK(counted::alive == 6);
        c.clear();
        CHECK(counted::alive == 3);
    }
    CHECK(counted::alive == 0);

    // what is dropped can be seen to on its way out
    coblru<std::string, std::unique_ptr<int>, 2> up;
    int                                          released = 0;
    auto release = [&](std::string const&, std::unique_ptr<int>& p) { released += *p; };
    CHECK(up.put("a", std::make_unique<int>(1), release) && up.put("b", std::make_unique<int>(2), release));
    CHECK(up.put("c", std::make_unique<int>(4), release) && released == 1);
    CHECK(up.with("b", [](std::unique_ptr<int>& p) { *p = 20; }) && up.put("d", std::make_unique<int>(8), release));
    CHECK(released == 5 && up.contains("b") && up.contains("d"));

    coblru<int, int, 1> one;
    CHECK(one.put(1, 1) && one.put(2, 2) && !one.contains(1) && one.get<std::optional>(2) == 2 && one.erase(2));

    struct unset {};
    atblru<int, int, unset> none;
    CHECK(!none.put(1, 1) && none.empty() && none.full() && !none.get<std::optional>(1).has_value() && !none.erase(1));
}

int main()
{
    CHECK(atbdim<sized>::set(24));
    struct three {};
    CHECK(atbdim<three>::set(3));

    basics<coblru<std::string, int, 3>>();
    basics<atblru<std::string, int, three>>();
    against_model<coblru<int, int, 24>>(24, 40);
    against_model<coblru<int, int, 24, alike>>(24, 40);
    against_model<coblru<int, int, 7>>(7, 9);
    against_model<atblru<int, int, sized>>(24, 60);
    anytype();
    return 0;
}
