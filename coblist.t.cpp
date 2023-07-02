#include <iostream>
#include "coblist.hpp"

#include <optional>
#include <cassert>


template <class T, int N>
void print(coblist<T,N> const& l) {
    for (auto& x: l) {
        std::cout << x << std::endl;
    }
}

void basic()
{
    coblist<int, 5> l;

    assert(l.empty());
    assert(l.size() == 0);
    assert(l.push_front(1));
    assert(!l.empty());
    assert(l.size() == 1);
    
    assert(l.push_front(2));
    assert(l.push_front(3));
    assert(l.push_front(4));
    assert(l.push_front(5));
    assert(l.size() == 5);
    assert(!l.push_front(6));

    {
        int const exp[] = {5,4,3,2,1};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();
    {
        int const exp[] = {1,2,3,4,5};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();

    auto x = l.pop_front<std::optional>();
    assert(x.has_value());
    assert(*x == 5);
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 4));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 3));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 2));
    x = l.pop_front<std::optional>();
    assert(x.has_value() && (*x == 1));
    assert(!l.pop_front<std::optional>().has_value());
    assert(l.empty());

    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(13));
    assert(l.push_front(14));
    {
        int const exp[] = {14,13,12,11};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.reverse();
    {
        int const exp[] = {11,12,13,14};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
    l.clear();
    assert(l.empty());
}


void erase()
{
    coblist<int, 5> l;

    assert(l.push_front(1));
    assert(l.push_front(2));
    assert(l.push_front(3));
    assert(l.push_front(4));
    assert(l.push_front(5));

    auto r = l.erase(l.begin());
    assert(r == l.begin());
    assert(l.size() == 4);
    r = l.erase(++r);
    assert(3 == l.size());
    l.erase(++r);
    assert(2 == l.size());

    {
        int const exp[] = {4,2};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


void remove()
{
    coblist<int, 6> l;

    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(11));
    assert(l.push_front(14));

    assert(2 == l.remove(11));
    assert(0 == l.remove(11));
    assert(1 == l.remove(12));

    assert(l.size() == 1);
    auto x = l.pop_front<std::optional>();
    assert(x.has_value());
    assert(*x == 14);
    assert(l.empty());
}


void unique()
{
    coblist<int, 9> l;

    assert(l.push_front(11));
    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(15));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(14));

    assert(4 == l.unique());
    assert(0 == l.unique());

    {
        int const exp[] = {14,15,14,12,11};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}

void sort()
{
    coblist<int, 9> l;

    assert(l.push_front(11));
    assert(l.push_front(11));
    assert(l.push_front(12));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(15));
    assert(l.push_front(14));
    assert(l.push_front(14));
    assert(l.push_front(14));

    l.sort();

    {
        int const exp[] = {11,11,12,14,14,14,14,14,15};
        assert(std::equal(l.begin(), l.end(), std::begin(exp), std::end(exp)));
    }
}


int main()
{
    basic();
    erase();
    remove();
    unique();
    sort();

    return 0;
}
