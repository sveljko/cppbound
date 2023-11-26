#include <iostream>

#include "cobarray.hpp"

template <int N, int D, int G>
void print(std::ostream& out, cobarray<cobint<D, G>, N> const &l)
{
    for (auto &x : l)
    {
        std::cout << x.get() << " ";
    }
    std::cout << "." << std::endl;
}

template <int N, int D, int G>
void print(cobarray<cobint<D, G>, N> const &l) {
    print(std::cout, l);
}

#include "cobskiplist.hpp"

#include <optional>
#include <cassert>


template <class T, int N>
void print(cobskiplist<T,N> const& l) {
    for (auto& x: l) {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
}

void basic()
{
    cobskiplist<int, 8> l;

    assert(l.empty());
    assert(l.size() == 0);
    
    assert(l.insert(1).second);
    assert(!l.empty());
    assert(l.size() == 1);
    assert(!l.insert(1).second);

    std::cout.flush();

    assert(l.insert(2).second);
    assert(l.insert(3).second);
    assert(l.insert(4).second);
    assert(l.insert(5).second);
    assert(l.size() == 5);

    assert(l.insert(6).second);
    assert(l.insert(7).second);
    assert(l.insert(8).second);
    assert(!l.insert(9).second);
    assert(l.size() == 8);

    {
        int const exp[] = {1,2,3,4,5,6,7,8};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }

    assert(l.erase(555) == 0);
    assert(l.erase(3) == 1);
    assert(l.erase(1) == 1);
    assert(l.erase(8) == 1);
    assert(l.erase(4) == 1);
    assert(l.erase(2) == 1);
    assert(l.erase(5) == 1);
    assert(l.erase(6) == 1);
    assert(l.erase(7) == 1);

    assert(l.empty());
}



int main()
{
    basic();

    return 0;
}
