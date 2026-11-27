/* Allocation time bound timer lean ring. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTMRLEANRING)
#define      INC_ATBTMRLEANRING


#include "atbbound.hpp"
#include "cobtmrleanring.hpp"


// A `cobtmrleanring` for as many timers as the dimension of `Tag`, set at run
// time. Like there, there is room for one more, and no index of a timer.
template <class ID, class Tag, class U, class NULIFY>
using atbtmrleanring = bndtmrleanring<ID, atbbound<Tag>, U, NULIFY>;


#endif // !defined(INC_ATBTMRLEANRING)
