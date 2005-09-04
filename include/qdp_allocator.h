// -*- C++ -*-
// $Id: qdp_allocator.h,v 1.4 2005-09-04 03:31:48 edwards Exp $

/*! \file
 * \brief Catch-alls for all memory allocators
 */

#ifndef QDP_ALLOCATOR
#define QDP_ALLOCATOR

/*! QDP Allocator
 *  A raw memory allocator for QDP, for particular use 
 *  with OLattice Objects. The pointers returned by allocate
 *  are all allocated with the correct alignment
 *  On normal targets this should be QDP_ALIGNMENT
 *  whereas on other targets (QCDOC) this should be the default
 *  alignment
 */
using namespace std;

QDP_BEGIN_NAMESPACE(QDP);
QDP_BEGIN_NAMESPACE(Allocator);

enum MemoryPoolHint { DEFAULT, FAST };

QDP_END_NAMESPACE(); // Allocator

// Memory movement hints
QDP_BEGIN_NAMESPACE(Hints);

//! Hint to move a generic object of type T to fast memory.
/*!
 * \ingroup QDP Memory management hints
 * 
 * This is a catch all function for objects that do not support
 * memory management hints. It does nothing and had better be inlined
 * or it can be detremental to my health
 *
 * \param x   The object for which the hint is meant
 * \param copy Whether to copy the object's slow memory contents to 
 *             its new fast memory home. Default is no.
 */
template<typename T> 
inline
void moveToFastMemoryHint(T& x, bool copy=false) {}

//! Hint to return a generic object of type T from fast memory.
/*!
 * \ingroup QDP Memory management hints
 * 
 * This is a catch all function for objects that do not support
 * memory management hints. It does nothing and had better be inlined
 * or it can be detremental to my health
 *
 * \param x   The object for which the hint is meant
 * \param copy Whether to copy the object's fast memory contents to 
 *             back to its slow memory home. Default is no.
 */
template<typename T>
inline
void revertFromFastMemoryHint(T& x, bool copy=false) {}


QDP_END_NAMESPACE(); // Hints

QDP_END_NAMESPACE(); // QDP

#ifdef QDP_USE_QCDOC
// Include the QCDOC specialisation
#include "qdp_qcdoc_allocator.h"
#else
// Include the default specialisation
#include "qdp_default_allocator.h"
#endif


#endif
