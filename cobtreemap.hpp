/* Compile time bound map on a balanced search tree. For documentation, see README.md.
 * (C) Srdjan Veljkovic
 * License: MIT (see LICENSE)
 */

#if !defined(INC_COBTREEMAP)
#define      INC_COBTREEMAP


#include "cobmap.hpp"
#include "cobtree.hpp"


// The map that is a view of the tree: the interface of `cobmap`, with all of
// it taking O(log n) steps in the worst case
template <class K, class V, class B, class CMP = cobhlp::less<K>, bool CHECKED = false, bool HIVE = false>
using bndtreemap = bndmapview<K, V, bndtree<cobhlp::keyval<K, V>, B, cobhlp::bykey<K, V, CMP>, CHECKED, HIVE>>;

template <class K, class V, int N, class CMP = cobhlp::less<K>>
using cobtreemap = bndtreemap<K, V, cobbound<N>, CMP>;

// The one with checked links
template <class K, class V, int N, class CMP = cobhlp::less<K>>
using cobcheckedtreemap = bndtreemap<K, V, cobbound<N>, CMP, true>;

// The ones where which slots hold an element is kept as a bit each (see `cobhive`)
template <class K, class V, int N, class CMP = cobhlp::less<K>>
using cobhivetreemap = bndtreemap<K, V, cobbound<N>, CMP, false, true>;
template <class K, class V, int N, class CMP = cobhlp::less<K>>
using cobcheckedhivetreemap = bndtreemap<K, V, cobbound<N>, CMP, true, true>;


#endif // !defined(INC_COBTREEMAP)
