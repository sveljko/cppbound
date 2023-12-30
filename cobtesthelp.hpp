#if !defined(INC_COBTESTHELP)
#define INC_COBTESTHELP


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


#endif // !defined(INC_COBTESTHELP)
