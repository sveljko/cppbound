/* Allocation time bound doubly linked list. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBLIST)
#define      INC_ATBLIST


#include "atbbound.hpp"
#include "coblist.hpp"


// A `coblist` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atblist = bndlist<T, atbbound<Tag>>;

// The one with checked links
template <class T, class Tag>
using atbcheckedlist = bndlist<T, atbbound<Tag>, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class T, class Tag>
using atbhivelist = bndlist<T, atbbound<Tag>, false, true>;
template <class T, class Tag>
using atbcheckedhivelist = bndlist<T, atbbound<Tag>, true, true>;


#endif // !defined(INC_ATBLIST)
