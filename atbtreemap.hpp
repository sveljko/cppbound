/* Allocation time bound map on a balanced search tree. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_ATBTREEMAP)
#define      INC_ATBTREEMAP


#include "atbbound.hpp"
#include "cobtreemap.hpp"


// A `cobtreemap` whose capacity is the dimension of `Tag`, set at run time
template <class K, class V, class Tag, class CMP = cobhlp::less<K>>
using atbtreemap = bndtreemap<K, V, atbbound<Tag>, CMP>;

// The one with checked links
template <class K, class V, class Tag, class CMP = cobhlp::less<K>>
using atbcheckedtreemap = bndtreemap<K, V, atbbound<Tag>, CMP, true>;

// The ones where which slots hold an element is kept as a bit each (see `atbhive`)
template <class K, class V, class Tag, class CMP = cobhlp::less<K>>
using atbhivetreemap = bndtreemap<K, V, atbbound<Tag>, CMP, false, true>;
template <class K, class V, class Tag, class CMP = cobhlp::less<K>>
using atbcheckedhivetreemap = bndtreemap<K, V, atbbound<Tag>, CMP, true, true>;


#endif // !defined(INC_ATBTREEMAP)
