/* Allocation time bound flat map. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBFLATMAP)
#define      INC_ATBFLATMAP


#include "atbbound.hpp"
#include "cobflatmap.hpp"


// A `cobflatmap` whose capacity is the dimension of `Tag`, set at run time
template <class K, class V, class Tag, class CMP = cobhlp::less<K>>
using atbflatmap = bndflatmap<K, V, atbbound<Tag>, CMP>;


#endif // !defined(INC_ATBFLATMAP)
