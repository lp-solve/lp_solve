/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1996      Roland Wunderling                              */
/*                  1996-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: spxalloc.h,v 1.23 2010/09/16 17:45:03 bzfgleix Exp $"

/**@file  spxalloc.h
 * @brief Memory allocation routines.
 */
#ifndef _SPXALLOC_H_
#define _SPXALLOC_H_

#include <iostream>
#include <stdlib.h>
#include <assert.h>

#include "spxdefines.h"
#include "spxout.h"

#include "exceptions.h"

namespace soplex
{
/**@name    Memory allocation routines
 * @ingroup Elementary
 * Here we have cover functions for malloc/realloc/free, to make sure
 * that we allays succeed. Otherwise an exception is thrown.
 *
 * We use templates to get the types right, otherwise casts would have 
 * been neccessary.
 */
//@{
/**@brief Allocate memory.
 * @param p some pointer
 * @param n the number of elements \p p will point to.
 * @throw SPxMemoryException if memory could not be allocated.
 */
template <class T>
inline void spx_alloc(T& p, int n)
{
   assert(p == 0);
   assert(n >= 0);

   if (n == 0)
      n = 1;
   
   p = reinterpret_cast<T>(malloc(sizeof(*p) * n));

   if (0 == p)
   {
      MSG_ERROR( spxout << "EMALLC01 malloc: Out of memory - cannot allocate " 
                        << sizeof(*p) * n << " bytes" << std::endl; )
      throw(SPxMemoryException("XMALLC01 malloc: Could not allocate enough memory") );
   }
}

/**@brief Change amount of allocated memory.
 * @param p some pointer
 * @param n the number of elements p should point to.
 * @throw SPxMemoryException if memory could not be allocated.
 */
template <class T>
inline void spx_realloc(T& p, int n)
{
   assert(n >= 0);

   if (n == 0)
      n = 1;

   T pp = reinterpret_cast<T>(realloc(p, sizeof(*p) * n));

   if (0 == pp)
   {
      MSG_ERROR( spxout << "EMALLC02 realloc: Out of memory - cannot allocate "
                        << sizeof(*p) * n << " bytes" << std::endl; )
      throw(SPxMemoryException("XMALLC02 realloc: Could not allocate enough memory") );
   }
   p=pp;
}

/// Release memory
template <class T>
inline void spx_free(T& p)
{
   assert(p != 0);
   free(p);
   
   p = 0;
}

//@}
} // namespace soplex


#endif // _SPXALLOC_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------
