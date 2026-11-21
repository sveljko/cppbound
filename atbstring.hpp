/* Allocation time bound string. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBSTRING)
#define      INC_ATBSTRING


#include "atbbound.hpp"
#include "cobstring.hpp"


// A `cobstring` whose capacity is the dimension of `Tag`, set at run time, or 255 if
// the dimension is greater than that
template <class Tag>
using atbstring = bndstring<atbbound<Tag>>;


#endif // !defined(INC_ATBSTRING)
