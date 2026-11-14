#include "cobbitset.hpp"
#include "cobtesthelp.hpp"

#include <bitset>
#include <optional>
#include <random>
#include <vector>

constexpr bool compiletime()
{
    cobbitset<10> b;
    b.set(cobbitset<10>::index::greatest());
    b.set(cobbitset<10>::index::smallest());
    return b.count() == 2 && b.test(cobbitset<10>::index::greatest());
}
static_assert(compiletime());

template <int N> void sized()
{
    using bs  = cobbitset<N>;
    using idx = typename bs::index;
    bs b;
    CHECK(b.none() && !b.any() && !b.all() && b.count() == 0 && bs::size() == N);
    b.set();
    CHECK(b.all() && b.count() == static_cast<unsigned>(N)); // the tail is masked
    b.reset();
    CHECK(b.none());
    b.set(idx::greatest());
    CHECK(b.test(idx::greatest()) && b.count() == 1 && b.any());
    b.flip(idx::greatest());
    CHECK(b.none());
    b.set(idx::smallest(), true);
    b.set(idx::smallest(), false);
    CHECK(!b.test(idx::smallest()));
    b.set(idx::smallest());
    b.reset(idx::smallest());
    CHECK(b.none());

    b.flip();
    CHECK(b.all());
    bs c = ~b;
    CHECK(c.none() && c.count() == 0);
    c = ~c;
    CHECK(c.all() && c == b);
    CHECK((b & c) == b && (b ^ c).none() && (b | bs{}) == b);

    CHECK(b.maybe_set(N - 1, false) && !b.test(idx::greatest()));
    CHECK(!b.maybe_set(N) && !b.maybe_set(-1));

    // walking
    bs w;
    for (int i = 0; i < N; i += 3) { w.maybe_set(i); }
    std::vector<int> a, v;
    w.for_each([&](idx i) { a.push_back(i.get()); });
    for (auto i = w.template first<std::optional>(); i; i = w.template next<std::optional>(*i)) {
        v.push_back(i->get());
    }
    CHECK(a == v);
    CHECK(a.size() == w.count());
    CHECK(!bs{}.template first<std::optional>().has_value());
    CHECK(!w.template next<std::optional>(idx::greatest()).has_value());
}

template <int N> void model()
{
    cobbitset<N>  b;
    std::bitset<N> ref;
    std::mt19937  gen(N);
    for (int op = 0; op < 3000; ++op) {
        const int i = static_cast<int>(gen() % N);
        typename cobbitset<N>::index idx;
        idx.be(i);
        switch (gen() % 6) {
        case 0: b.set(idx); ref.set(static_cast<std::size_t>(i)); break;
        case 1: b.reset(idx); ref.reset(static_cast<std::size_t>(i)); break;
        case 2: b.flip(idx); ref.flip(static_cast<std::size_t>(i)); break;
        case 3: if (gen() % 40 == 0) { b.flip(); ref.flip(); } break;
        case 4: if (gen() % 40 == 0) { b.set(); ref.set(); } break;
        default: if (gen() % 40 == 0) { b = ~b; ref = ~ref; } break;
        }
        CHECK(b.count() == ref.count());
        CHECK(b.all() == ref.all() && b.any() == ref.any() && b.none() == ref.none());
        CHECK(b.test(idx) == ref.test(static_cast<std::size_t>(i)));
    }
}

int main()
{
    sized<1>();
    sized<5>();
    sized<64>();
    sized<65>();
    sized<130>();
    model<130>();
    model<64>();

    return 0;
}
