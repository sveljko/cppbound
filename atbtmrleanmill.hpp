/* Allocation time bound timer lean mill. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTMRLEANMILL)
#define      INC_ATBTMRLEANMILL


#include "atbbound.hpp"
#include "cobtmrleanmill.hpp"


// A `cobtmrleanmill` for as many timers as the dimension of `Tag`, set at run
// time. Like there, each spoke has room for its share of them and one more;
// if there are no more timers than spokes, there is no room at all.
template <class ID, class Tag, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
using atbtmrleanmill = bndtmrleanmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, NULIFY, LVL, DIM>;

// The one with checked indices
template <class ID, class Tag, class U, class NULIFY, unsigned LVL = 5, unsigned DIM = 64>
using atbcheckedtmrleanmill =
    bndtmrleanmill<ID, atbbound<atbpart<Tag, 1, LVL * DIM, 1, LVL * DIM + 1>>, U, NULIFY, LVL, DIM, true>;


#endif // !defined(INC_ATBTMRLEANMILL)
