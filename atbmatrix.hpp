/* Allocation time bound matrix. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBMATRIX)
#define      INC_ATBMATRIX


#include "atbbound.hpp"
#include "cobmatrix.hpp"


// A `cobmatrix` whose rows and columns are as many as the dimensions of the tags, set at run time
template <class T, class RowTag, class ColumnTag>
using atbmatrix = bndmatrix<T, atbbound<RowTag>, atbbound<ColumnTag>>;


#endif // !defined(INC_ATBMATRIX)
