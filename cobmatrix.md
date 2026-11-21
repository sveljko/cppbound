# Matrix

```cpp
#include "cobmatrix.hpp"    // or "atbmatrix.hpp"

cobmatrix<T, M, N>          atbmatrix<T, RowTag, ColumnTag>
```

A two dimensional array of `M` rows and `N` columns, all the elements
of which are there for as long as the matrix is. It is to
`cobarray<cobarray<T, N>, M>` what a matrix is to an array of arrays:
the elements are in one block, and getting at one does not copy a
row.

There is no Standard counterpart (the nearest is `std::mdspan`, which
is a view and owns nothing). It is made to be like the array
([cobarray.md](cobarray.md)), so that what you know of one goes for
the other.

No access is ever out of the matrix, and that takes no check at run
time: an element is named by an index of the row and an index of the
column, each known at compile time to be within bounds.

All of `cobmatrix` can be used in constant expressions, except for
the iterators.


# The index

```cpp
using row    = /* cobint<0, M - 1>, or atbidx<RowTag> */;
using column = /* cobint<0, N - 1>, or atbidx<ColumnTag> */;

struct index {
    row    irow;
    column icolumn;
};
```

An index is a row and a column. It is written with braces where it is
used:

```cpp
cobmatrix<int, 3, 4> m;
m.set({ cobic<2>, cobic<3> }, 7);       // the last element
m.set({ cobic<3>, cobic<0> }, 7);       // does not compile: no row 3
```

See [cobi.md](cobi.md) and [atbdim.md](atbdim.md) for how the row and
the column are made.


# Construction

```cpp
cobmatrix<int, 3, 4> m;
```

There is only the default constructor. The elements are default
initialised: objects are default constructed, but numbers and the
like have no particular value until they are set. Fill the matrix
(`fill()`, `filled()` or `generate()`) before reading from it. (This
is unlike `cobarray`, whose elements start as 0.)

`T` has to have a default constructor.


# Access an element

```cpp
constexpr T get(index i) const;
constexpr T set(index i, T const& t);
constexpr T set(index i, T&& t);
```

`get()` returns a copy of the element at `i`.

`set()` assigns `t` to the element at `i`, and returns a copy of the
element as it then is.


# Access a member of an element

```cpp
template <class V> constexpr V get(index i, V T::*m) const;
template <class V> constexpr V set(index i, V T::*m, V const& v);
constexpr taken  grab(index i);
constexpr ctaken grab(index i) const;
```

For when `T` is a struct or a class.

`get(i, m)` returns a copy of the member `m` of the element at `i`,
and `set(i, m, v)` assigns `v` to that member and returns a copy of
it as it then is. `m` is a pointer to a data member.

`grab(i)` returns an object whose only use is `->`, to get at a member
of the element at `i`. It can't be copied, moved or kept.

```cpp
struct cell {
    int  value = 0;
    bool seen  = false;
};
cobmatrix<cell, 8, 8> board;
board.set({ cobic<0>, cobic<1> }, &cell::value, 5);
board.grab({ cobic<0>, cobic<1> })->seen = true;
```


# Set an element by numbers

```cpp
template <class U1, class U2> constexpr bool maybe_set(U1 m, U2 n, T const& t);
```

Assigns `t` to the element in the row number `m` and the column
number `n`, where `m` and `n` are any integers. Returns `false`, and
does nothing, if there is no such element.


# A whole row

```cpp
constexpr /* array of N */ get_row(row r) const;
constexpr void set_row(row r, /* array of N */ const& v);
```

`get_row(r)` returns a copy of the row `r` as an array: a
`cobarray<T, N>` for `cobmatrix`, an `atbarray<T, ColumnTag>` for
`atbmatrix`.

`set_row(r, v)` assigns the elements of such an array to the row `r`.


# All the elements

```cpp
static constexpr bndmatrix filled(T const& v);
template <class F> static constexpr bndmatrix generate(F f);
constexpr void fill(T const& v);
```

`filled(v)` returns a matrix with every element a copy of `v`, and
`fill(v)` makes every element of this one a copy of `v`.

`generate(f)` returns a matrix where the element at each index `i` is
`f(i)`. `f` is called with the `index` (the struct above), row by row.

```cpp
using table = cobmatrix<int, 10, 10>;
auto times = table::generate([](table::index i) { return (i.irow.get() + 1) * (i.icolumn.get() + 1); });
```


# The first and the last element

```cpp
constexpr T front() const;
constexpr T back() const;
```

A copy of the element in the first row and the first column, and of
the one in the last row and the last column.


# Size

```cpp
static constexpr auto rows();
static constexpr auto columns();
constexpr auto size() const noexcept;
constexpr auto max_size() const noexcept;
constexpr bool empty() const noexcept;
```

`rows()` is `M` and `columns()` is `N`. `size()` and `max_size()` are
the number of elements, `M * N`. `empty()` is `true` only for an
`atbmatrix` with no rows or no columns.


# Compare and swap

```cpp
constexpr bool operator==(bndmatrix const& x) const;
constexpr void swap(bndmatrix& x);
```

Two matrices are equal if all their elements are. `swap()` swaps the
elements one by one.


# Iterators

```cpp
iterator       begin();
iterator       end();
const_iterator begin() const;
const_iterator end() const;
const_iterator cbegin() const;
const_iterator cend() const;
```

Random access, going over all the elements row by row: the first row
first, each row from its first column to its last. So the matrix can
be given to range-for and to the Standard algorithms as one sequence
of `M * N` elements.

They behave as [common.md](common.md) says: an iterator can't be
moved out of the matrix, and `end()` can be dereferenced, giving the
"safety spot". As in the array, the safety spot is a part of the
matrix: there is one element more than `M * N`.

There is no `irange()`. To go over the matrix by indices, go over
the rows and the columns:

```cpp
cobmatrix<int, 3, 4> m;
for (auto r : cobirange<int, 0, 2>{}) {
    for (auto c : cobirange<int, 0, 3>{}) {
        m.set({ r, c }, 0);
    }
}
```

For an `atbmatrix`, that is `atbidx<RowTag>::range()` and
`atbidx<ColumnTag>::range()`.


# `atbmatrix`

As for the other `atb` variants ([atbdim.md](atbdim.md)). The number
of rows is the dimension of `RowTag` and the number of columns that
of `ColumnTag`; the two may be the same tag. The elements are on the
heap, value initialised (numbers are 0), and it is not usable in
constant expressions.


# What it takes

Room for `M * N + 1` objects of type `T`. Nothing else.
