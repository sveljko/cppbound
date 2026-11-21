/* Allocation time bound singly linked list. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBFWDLIST)
#define      INC_ATBFWDLIST


#include "atbbound.hpp"
#include "cobfwdlist.hpp"


// A `cobfwdlist` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag>
using atbfwdlist = bndfwdlist<T, atbbound<Tag>>;

// The one with checked links
template <class T, class Tag>
using atbcheckedfwdlist = bndfwdlist<T, atbbound<Tag>, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class T, class Tag>
using atbhivefwdlist = bndfwdlist<T, atbbound<Tag>, false, true>;
template <class T, class Tag>
using atbcheckedhivefwdlist = bndfwdlist<T, atbbound<Tag>, true, true>;


#endif // !defined(INC_ATBFWDLIST)
