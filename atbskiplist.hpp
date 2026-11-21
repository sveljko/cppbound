/* Allocation time bound skiplist. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBSKIPLIST)
#define      INC_ATBSKIPLIST


#include "atbbound.hpp"
#include "cobskiplist.hpp"


// A `cobskiplist` whose capacity is the dimension of `Tag`, set at run time
template <class T, class Tag, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using atbskiplist = bndskiplist<T, atbbound<Tag>, CMP, RNG>;

// The one with checked links
template <class T, class Tag, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using atbcheckedskiplist = bndskiplist<T, atbbound<Tag>, CMP, RNG, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class T, class Tag, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using atbhiveskiplist = bndskiplist<T, atbbound<Tag>, CMP, RNG, false, true>;
template <class T, class Tag, class CMP = cobhlp::less<T>, class RNG = cobhlp::xorshift32>
using atbcheckedhiveskiplist = bndskiplist<T, atbbound<Tag>, CMP, RNG, true, true>;


#endif // !defined(INC_ATBSKIPLIST)
