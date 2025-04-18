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
#pragma ident "@(#) $Id: spxvectorst.cpp,v 1.19 2010/09/16 17:45:04 bzfgleix Exp $"

//#define DEBUGGING 1

#include <assert.h>
#include <iostream>

#include "spxdefines.h"
#include "spxvectorst.h"

namespace soplex
{

void SPxVectorST::setupWeights(SPxSolver& base)
{
   if (state == PVEC)
   {
      if (vec.dim() != base.nCols())
      {
         SPxWeightST::setupWeights(base);
         return;
      }

      const Vector& obj = base.maxObj();
      Real eps = base.epsilon();
      Real bias = 10000 * eps;
      Real x, y;
      int i;

      MSG_DEBUG( spxout << "DVECST01 colWeight[]: "; )
      for (i = base.nCols(); i--;)
      {
         x = vec[i] - base.SPxLP::lower(i);
         y = base.SPxLP::upper(i) - vec[i];
         if (x < y)
         {
            colWeight[i] = -x - bias * obj[i];
            colUp[i] = 0;
         }
         else
         {
            colWeight[i] = -y + bias * obj[i];
            colUp[i] = 1;
         }
         MSG_DEBUG( spxout << colWeight[i] << " "; )
      }
      MSG_DEBUG( spxout << std::endl << std::endl; )

      MSG_DEBUG( spxout << "DVECST02 rowWeight[]: "; )
      for (i = base.nRows(); i--;)
      {
         const SVector& row = base.rowVector(i);
         y = vec * row;
         x = (y - base.lhs(i));
         y = (base.rhs(i) - y);
         if (x < y)
         {
            rowWeight[i] = -x - eps * row.size() - bias * (obj * row);
            rowRight[i] = 0;
         }
         else
         {
            rowWeight[i] = -y - eps * row.size() + bias * (obj * row);
            rowRight[i] = 1;
         }
         MSG_DEBUG( spxout << rowWeight[i] << " "; )
      }
      MSG_DEBUG( spxout << std::endl; )
   }

   else if (state == DVEC)
   {
      if (vec.dim() != base.nRows())
      {
         SPxWeightST::setupWeights(base);
         return;
      }

      Real x, y, len;
      int i, j;
      for (i = base.nRows(); i--;)
         rowWeight[i] += fabs(vec[i]);

      for (i = base.nCols(); i--;)
      {
         const SVector& col = base.colVector(i);
         for (y = len = 0, j = col.size(); j--;)
         {
            x = col.value(j);
            y += vec[col.index(j)] * x;
            len += x * x;
         }
         if (len > 0)
            colWeight[i] += fabs(y / len - base.maxObj(i));
      }
   }
   else
      SPxWeightST::setupWeights(base);
}
} // namespace soplex

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------
