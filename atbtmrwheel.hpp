/* Allocation time bound timer wheel. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTMRWHEEL)
#define      INC_ATBTMRWHEEL


#include "atbbound.hpp"
#include "cobtmrwheel.hpp"


// A `cobtmrwheel` for as many timers as the dimension of `Tag`, set at run
// time. Like there, each spoke has room for twice its share of them.
template <class ID, class Tag, class U, unsigned SPOKES>
using atbtmrwheel = bndtmrwheel<ID, atbbound<atbpart<Tag, 2, SPOKES>>, U, SPOKES>;

// The one with checked indices
template <class ID, class Tag, class U, unsigned SPOKES>
using atbcheckedtmrwheel = bndtmrwheel<ID, atbbound<atbpart<Tag, 2, SPOKES>>, U, SPOKES, true>;

// The ones where which slots hold a timer is kept as a bit each (see `atbhive`)
template <class ID, class Tag, class U, unsigned SPOKES>
using atbhivetmrwheel = bndtmrwheel<ID, atbbound<atbpart<Tag, 2, SPOKES>>, U, SPOKES, false, true>;
template <class ID, class Tag, class U, unsigned SPOKES>
using atbcheckedhivetmrwheel = bndtmrwheel<ID, atbbound<atbpart<Tag, 2, SPOKES>>, U, SPOKES, true, true>;


#endif // !defined(INC_ATBTMRWHEEL)
