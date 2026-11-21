/* Allocation time bound timer mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTMRMILL)
#define      INC_ATBTMRMILL


#include "atbbound.hpp"
#include "cobtmrmill.hpp"


// A `cobtmrmill` for as many timers as the dimension of `Tag`, set at run time.
// The regular one: each spoke has room for its share of them and one more; if
// there are no more timers than spokes, there is no room at all.
// The exact one (`EXACT` being `true`): the room is shared by all the spokes.
template <class ID, class Tag, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using atbtmrmill = std::conditional_t<
    EXACT, bndtmrexactmill<ID, atbbound<Tag>, atbbound<atbpart<Tag, 1, 1, LVL * DIM>>, U, LVL, DIM>,
    bndtmrmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, LVL, DIM>>;

// The one with checked indices (which the exact mill has anyway)
template <class ID, class Tag, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using atbcheckedtmrmill = std::conditional_t<
    EXACT, bndtmrexactmill<ID, atbbound<Tag>, atbbound<atbpart<Tag, 1, 1, LVL * DIM>>, U, LVL, DIM>,
    bndtmrmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, LVL, DIM, true>>;

// The ones where which slots hold a timer is kept as a bit each
template <class ID, class Tag, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using atbhivetmrmill = std::conditional_t<
    EXACT, bndtmrexactmill<ID, atbbound<Tag>, atbbound<atbpart<Tag, 1, 1, LVL * DIM>>, U, LVL, DIM, true>,
    bndtmrmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, LVL, DIM, false, true>>;
template <class ID, class Tag, class U, unsigned LVL = 5, unsigned DIM = 64, bool EXACT = false>
using atbcheckedhivetmrmill = std::conditional_t<
    EXACT, bndtmrexactmill<ID, atbbound<Tag>, atbbound<atbpart<Tag, 1, 1, LVL * DIM>>, U, LVL, DIM, true>,
    bndtmrmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, LVL, DIM, true, true>>;


#endif // !defined(INC_ATBTMRMILL)
