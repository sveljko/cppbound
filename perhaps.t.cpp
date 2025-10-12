#include "perhaps.hpp"

#include <iostream>
#include <string>


template <class M> auto join(M&& m) {
    return m.and_then([](auto&& x) { return x; });
}

perhaps<int> operator+(const perhaps<int>& x, const perhaps<int>& y) {
    return x.and_then([&](int x) { return y.and_then([&](int y) { return perhaps<int>(x + y); }); });
}

struct mile {
        operator int() const {return x;}
	int x = 3;
};

int main() {
    using namespace std::string_literals;

    perhaps<int> x = 10;
    perhaps<int> y = 20;

    x.transform([](int x) { return (x == 10) ? "success"s : "fail"s; })
    .and_then([](std::string const& r) { std::cout << r << std::endl; });

    (x + y).and_then([](int r) { std::cout << r << std::endl; });

    std::cout << join(x + x + y) << std::endl;

    perhaps<mile> z;
    mile q;
    z.or_else([](mile r) {return perhaps<mile>{};}).value_or(q);
    std::cout << z.value_or(mile{5}) << std::endl;
}
