/* Allocation time bound balanced search tree. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTREE)
#define      INC_ATBTREE


#include "atbbound.hpp"
#include "cobtree.hpp"


// A `cobtree` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbtree = bndtree<T, atbbound<Tag>, CMP>;

// The one with checked links
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbcheckedtree = bndtree<T, atbbound<Tag>, CMP, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbhivetree = bndtree<T, atbbound<Tag>, CMP, false, true>;
template <class T, class Tag, class CMP = cobhlp::less<T>>
using atbcheckedhivetree = bndtree<T, atbbound<Tag>, CMP, true, true>;


#endif // !defined(INC_ATBTREE)
