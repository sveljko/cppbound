/* Allocation time bound timer list. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTMRLIST)
#define      INC_ATBTMRLIST


#include "atbbound.hpp"
#include "cobtmrlist.hpp"


// A `cobtmrlist` of as many timers as the dimension of `Tag`, set at run time
template <class ID, class Tag, class U>
using atbtmrlist = bndtmrlist<ID, atbbound<Tag>, U>;

// The one with checked indices
template <class ID, class Tag, class U>
using atbcheckedtmrlist = bndtmrlist<ID, atbbound<Tag>, U, true>;

// The ones where which slots hold a timer is kept as a bit each (see `atbhive`)
template <class ID, class Tag, class U>
using atbhivetmrlist = bndtmrlist<ID, atbbound<Tag>, U, false, true>;
template <class ID, class Tag, class U>
using atbcheckedhivetmrlist = bndtmrlist<ID, atbbound<Tag>, U, true, true>;


#endif // !defined(INC_ATBTMRLIST)
