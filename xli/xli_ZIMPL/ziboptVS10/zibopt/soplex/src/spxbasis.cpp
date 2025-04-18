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
#pragma ident "@(#) $Id: spxbasis.cpp,v 1.74 2010/09/16 17:45:03 bzfgleix Exp $"

//#define DEBUGGING 1

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "spxdefines.h"
#include "spxbasis.h"
#include "didxset.h"
#include "dvector.h"
#include "spxsolver.h"
#include "mpsinput.h"
#include "message.h"
#include "spxout.h"
#include "exceptions.h"

namespace soplex
{
SPxBasis::Desc::Status
SPxBasis::dualStatus(const SPxColId& id) const
{
   METHOD( "SPxBasis::dualStatus()" );
   return dualColStatus(static_cast<SPxLP*>(theLP)->number(id));
}

SPxBasis::Desc::Status
SPxBasis::dualStatus(const SPxRowId& id) const
{
   METHOD( "SPxBasis::dualStatus()" );
   return dualRowStatus((static_cast<SPxLP*>(theLP))->number(id));
}

SPxBasis::Desc::Status
SPxBasis::dualRowStatus(int i) const
{
   METHOD( "SPxBasis::dualRowStatus()" );
   assert(theLP != 0);

   if (theLP->rhs(i) < infinity)
   {
      if (theLP->lhs(i) > -infinity)
      {
         if (theLP->lhs(i) == theLP->rhs(i))
            return Desc::D_FREE;
         else
            return Desc::D_ON_BOTH;
      }
      else
         return Desc::D_ON_LOWER;
   }
   else if (theLP->lhs(i) > -infinity)
      return Desc::D_ON_UPPER;
   else
      return Desc::D_UNDEFINED;
}

SPxBasis::Desc::Status
SPxBasis::dualColStatus(int i) const
{
   METHOD( "SPxBasis::dualColStatus()" );
   assert(theLP != 0);

   if (theLP->SPxLP::upper(i) < infinity)
   {
      if (theLP->SPxLP::lower(i) > -infinity)
      {
         if (theLP->SPxLP::lower(i) == theLP->SPxLP::upper(i))
            return Desc::D_FREE;
         else
            return Desc::D_ON_BOTH;
      }
      else
         return Desc::D_ON_LOWER;
   }
   else if (theLP->SPxLP::lower(i) > -infinity)
      return Desc::D_ON_UPPER;
   else
      return Desc::D_UNDEFINED;
}

void SPxBasis::loadMatrixVecs()
{
   METHOD( "SPxBasis::loadMatrixVecs()" );
   assert(theLP != 0);
   assert(theLP->dim() == matrix.size());

   MSG_INFO3( spxout << "IBASIS01 loadMatrixVecs() invalidates factorization" 
                        << std::endl; )

   int i;
   nzCount = 0;
   for (i = theLP->dim() - 1; i >= 0; --i)
   {
      matrix[i] = &theLP->vector(baseId(i));
      nzCount += matrix[i]->size();
   }
   matrixIsSetup = true;
   factorized = false;
   if (factor != 0)
      factor->clear();
}

bool SPxBasis::isDescValid(const Desc& ds)
{
   METHOD( "SPxBasis::isDescValid()" );
   assert(status() > NO_PROBLEM);
   assert(theLP != 0);

   if ( ds.nRows() != theLP->nRows() || ds.nCols() != theLP->nCols() )
      return false;

   for ( int row = ds.nRows()-1; row >= 0; --row )
   {
      // row is basic
      if ( ds.rowstat[row] > 0 )
      {
         if ( ds.rowstat[row] != dualRowStatus(row) )
            return false;
      }
      // row is nonbasic
      else
      {
         if ( (ds.rowstat[row] == Desc::P_FIXED && theLP->SPxLP::lhs(row) != theLP->SPxLP::rhs(row))
              || (ds.rowstat[row] == Desc::P_ON_UPPER && theLP->SPxLP::rhs(row) >= infinity)
              || (ds.rowstat[row] == Desc::P_ON_LOWER && theLP->SPxLP::lhs(row) <= -infinity)
              || (ds.rowstat[row] == Desc::P_FREE && (theLP->SPxLP::lhs(row) > -infinity || theLP->SPxLP::rhs(row) < infinity)) )
            return false;
      }
   }

   for ( int col = ds.nCols()-1; col >= 0; --col )
   {
      // col is basic
      if ( ds.colstat[col] > 0 )
      {
         if ( ds.colstat[col] !=  dualColStatus(col) )
            return false;
      }
      // col is nonbasic
      else
      {
         if ( (ds.colstat[col] == Desc::P_FIXED && theLP->SPxLP::lower(col) != theLP->SPxLP::upper(col))
              || (ds.colstat[col] == Desc::P_ON_UPPER && theLP->SPxLP::upper(col) >= infinity)
              || (ds.colstat[col] == Desc::P_ON_LOWER && theLP->SPxLP::lower(col) <= -infinity)
              || (ds.colstat[col] == Desc::P_FREE && (theLP->SPxLP::lower(col) > -infinity || theLP->SPxLP::upper(col) < infinity)) )
            return false;
      }
   }

   // basis descriptor valid w.r.t. bounds
   return true;
}

/*
    Loading a #Desc# into the basis can be done more efficiently, by
    explicitely programming both cases, for the rowwise and for the columnwise
    representation. This implementation hides this distingtion in the use of
    methods #isBasic()# and #vector()#.
 */
void SPxBasis::loadDesc(const Desc& ds)
{
   METHOD( "SPxBasis::loadDesc()" );
   assert(status() > NO_PROBLEM);
   assert(theLP != 0);
   assert(ds.nRows() == theLP->nRows());
   assert(ds.nCols() == theLP->nCols());

   SPxId none;
   int   i;
   int   j;

   MSG_INFO3( spxout << "IBASIS02 loading of Basis invalidates factorization" 
                        << std::endl; )

   lastin      = none;
   lastout     = none;
   lastidx     = -1;
   iterCount   = 0;
   updateCount = 0;

   if (&ds != &thedesc)
   {
      thedesc = ds;
      setRep();
   }

   assert(theLP->dim() == matrix.size());

   MSG_DEBUG( dump(); )

   nzCount = 0;
   for (j = i = 0; i < theLP->nRows(); ++i)
   {
      if (theLP->isBasic(thedesc.rowStatus(i)))
      {
         SPxRowId id = theLP->SPxLP::rId(i);
         theBaseId[j] = id;
         matrix[j] = &theLP->vector(id);
         nzCount += matrix[j++]->size();
      }
   }

   for (i = 0; i < theLP->nCols(); ++i)
   {
      if (theLP->isBasic(thedesc.colStatus(i)))
      {
         SPxColId id = theLP->SPxLP::cId(i);
         theBaseId[j] = id;
         matrix[j] = &theLP->vector(id);
         nzCount += matrix[j++]->size();      
      }
   }

   assert(j == matrix.size());

   matrixIsSetup = true;
   factorized = false;
   if (factor != 0)
      factor->clear();
}

void SPxBasis::setRep()
{
   METHOD( "SPxBasis::setRep()" );
   assert(theLP != 0);

   reDim();
   minStab = 0.0;

   if (theLP->rep() == SPxSolver::ROW)
   {
      thedesc.stat   = &thedesc.rowstat;
      thedesc.costat = &thedesc.colstat;
   }
   else
   {
      thedesc.stat   = &thedesc.colstat;
      thedesc.costat = &thedesc.rowstat;
   }
}

void SPxBasis::load(SPxSolver* lp)
{
   METHOD( "SPxBasis::load()" );
   assert(lp != 0);
   theLP = lp;

   setRep();

   addedRows(lp->nRows());
   addedCols(lp->nCols());

   setStatus(REGULAR);

   loadDesc(thedesc);
}

void SPxBasis::loadSolver(SLinSolver* p_solver, const bool destroy)
{
   METHOD( "SPxBasis::loadSolver()" );

   assert(!freeSlinSolver || factor != 0);

   MSG_INFO3( spxout << "IBASIS03 loading of Solver invalidates factorization" 
                        << std::endl; )


   if(freeSlinSolver)
   {
      delete factor;
      factor = 0;
   }

   factor = p_solver;
   factorized = false;
   factor->clear();
   freeSlinSolver = destroy;
}




/** 
 *  The specification is taken from the
 *
 *  ILOG CPLEX 7.0 Reference Manual, Appendix E, Page 543.
 *
 *  This routine should read valid BAS format files. 
 *
 *  @return true if the file was read correctly.
 *
 *  Here is a very brief outline of the format:
 *
 *  The format is in a form similar to an MPS file. The basic assumption is that all (column)
 *  variables are nonbasic at their lower bound and all row (variables) are basic; only the
 *  differences to this rule are given. Each data line contains an indicator, a variable name and
 *  possibly a row/constraint name. The following meaning applies with respect to the indicators:
 *
 *  - XU: the variable is basic, the row is nonbasic at its upper bound 
 *  - XL: the variable is basic, the row is nonbasic at its lower bound 
 *  - UL: the variable is nonbasic and at its upper bound
 *  - LL: the variable is nonbasic and at its lower bound
 *
 *  The CPLEX format contains an additional indicator 'BS', but this is unsupported here.
 */
bool SPxBasis::readBasis(
   std::istream&  is, 
   const NameSet* rowNames, 
   const NameSet* colNames)
{
   METHOD( "SPxBasis::readBasis()" );
   assert(theLP != 0);

   /* prepare names */
   const NameSet* rNames = rowNames;
   const NameSet* cNames = colNames;

   if ( colNames == 0 )
   {
      int nCols = theLP->nCols();
      std::stringstream name;

      NameSet* p_colNames = new NameSet();
      p_colNames->reMax(nCols);
      for (int j = 0; j < nCols; ++j)
      {
         name << "x" << j << "_";
         DataKey key = theLP->colId(j);
         p_colNames->add(key, name.str().c_str());
      }
      cNames = p_colNames;
   }

   if ( rNames == 0 )
   {
      int nRows = theLP->nRows();
      std::stringstream name;

      NameSet* p_rowNames = new NameSet();
      p_rowNames->reMax(nRows);
      for (int i = 0; i < nRows; ++i)
      {
         name << "C" << i << "_";
         DataKey key = theLP->rowId(i);
         p_rowNames->add(key, name.str().c_str());
      }
      rNames = p_rowNames;
   }

   /* load default basis if necessary */
   if (status() == NO_PROBLEM)
      load(theLP);

   /* initialize with standard settings */
   Desc l_desc(thedesc);

   for (int i = 0; i < theLP->nRows(); i++)
      l_desc.rowstat[i] = dualRowStatus(i);

   for (int i = 0; i < theLP->nCols(); i++)
   {
      if (theLP->SPxLP::lower(i) == theLP->SPxLP::upper(i))
         l_desc.colstat[i] = Desc::P_FIXED;
      else
         l_desc.colstat[i] = Desc::P_ON_LOWER;
   }

   MPSInput mps(is);

   if (mps.readLine() && (mps.field0() != 0) && !strcmp(mps.field0(), "NAME"))
   {
      while (mps.readLine())
      {
         int c = -1;
         int r = -1;

         if ((mps.field0() != 0) && !strcmp(mps.field0(), "ENDATA"))
         {
            mps.setSection(MPSInput::ENDATA);
            break;
         }
         if ((mps.field1() == 0) || (mps.field2() == 0))
            break;

         if ((c = cNames->number(mps.field2())) < 0)
            break;

         if (*mps.field1() == 'X')
            if (mps.field3() == 0 || (r = rNames->number(mps.field3())) < 0)
               break;

         if (!strcmp(mps.field1(), "XU"))
         {
            l_desc.colstat[c] = dualColStatus(c);
            if ( theLP->SPxLP::lhs(r) == theLP->SPxLP::rhs(r) )
               l_desc.rowstat[r] = Desc::P_FIXED;
            else
               l_desc.rowstat[r] = Desc::P_ON_UPPER;
         }
         else if (!strcmp(mps.field1(), "XL"))
         {
            l_desc.colstat[c] = dualColStatus(c);
            if ( theLP->SPxLP::lhs(r) == theLP->SPxLP::rhs(r) )
               l_desc.rowstat[r] = Desc::P_FIXED;
            else
               l_desc.rowstat[r] = Desc::P_ON_LOWER;
         }
         else if (!strcmp(mps.field1(), "UL"))
         {
            l_desc.colstat[c] = Desc::P_ON_UPPER;
         }
         else if (!strcmp(mps.field1(), "LL"))
         {
            l_desc.colstat[c] = Desc::P_ON_LOWER;
         }
         else
         {
            mps.syntaxError();
            break;
         }
      }
   }
   if (!mps.hasError())
   {
      if (mps.section() == MPSInput::ENDATA)
      {
         // force basis to be different from NO_PROBLEM
         // otherwise the basis will be overwritten at later stages.
         setStatus(REGULAR);
         loadDesc(l_desc);
      }
      else
         mps.syntaxError();
   }

   if ( rowNames == 0 )
      delete rNames;
   if ( colNames == 0 )
      delete cNames;

#ifndef NDEBUG
   thedesc.dump();
#endif

   return !mps.hasError();
}


/* Get row name - copied from spxmpswrite.cpp 
 *
 * @todo put this in a common file and unify accross different formats (mps, lp, basis).
 */
static const char* getRowName(
   const SPxLP*   lp,
   int            idx,
   const NameSet* rnames, 
   char*          buf)
{
   assert(buf != 0);
   assert(idx >= 0);
   assert(idx < lp->nRows());

   if (rnames != 0) 
   {
      DataKey key = lp->rId(idx);

      if (rnames->has(key))
         return (*rnames)[key];
   }
   std::sprintf(buf, "C%d_", idx);
   
   return buf;
}
   
/* Get column name - copied from spxmpswrite.cpp 
 *
 * @todo put this in a common file and unify accross different formats (mps, lp, basis).
 */
static const char* getColName(
   const SPxLP*   lp,
   int            idx,
   const NameSet* cnames, 
   char*          buf)
{
   assert(buf != 0);
   assert(idx >= 0);
   assert(idx < lp->nCols());

   if (cnames != 0) 
   {
      DataKey key = lp->cId(idx);

      if (cnames->has(key))
         return (*cnames)[key];
   }
   std::sprintf(buf, "x%d_", idx);
   
   return buf;
}

/* writes a file in MPS basis format to \p os.
 *
 * See SPxBasis::readBasis() for a short description of the format.
 */
void SPxBasis::writeBasis(
   std::ostream&  os, 
   const NameSet* rowNames, 
   const NameSet* colNames 
   ) const
{
   METHOD( "SPxBasis::writeBasis()" );
   assert(theLP != 0);

   if (theLP->rep() == SPxSolver::ROW)
   {
      MSG_ERROR( spxout << "EBASIS09 writing basis for row representation "
                        << "not yet implemented!" << std::endl; )
      return;
   }
   assert(theLP->rep() == SPxSolver::COLUMN);

   os.setf(std::ios::left);
   os << "NAME  soplex.bas\n";

   /* do not write basis if there is none */
   if (status() == NO_PROBLEM)
   {
      os << "ENDATA" << std::endl;
      return;
   }

   /* start writing */
   char buf[255];
   int row = 0;
   for (int col = 0; col < theLP->nCols(); col++)
   {
      if ( theLP->isBasic( thedesc.colStatus( col ))) 
      {
         /* Find non basic row */
         for (; row < theLP->nRows(); row++)
         {
            if ( !theLP->isBasic( thedesc.rowStatus( row )))
               break;
         }

         assert( row != theLP->nRows() );

         os << ( thedesc.rowStatus( row ) == Desc::P_ON_UPPER ? " XU " : " XL " )
            << std::setw(8) << getColName(theLP, col, colNames, buf);

         /* break in two parts since buf is reused */
         os << "       " 
            << getRowName(theLP, row, rowNames, buf)
            << std::endl;

         row++;
      }
      else
      {
         if ( thedesc.colStatus( col ) == Desc::P_ON_UPPER )
         {
            os << " UL "
               << getColName(theLP, col, colNames, buf)
               << std::endl;
         }
         else
         {
            /* Default is all slacks basic, all variables on lower bound,
             * nothing to do in this case.
             * Non basic free variable should better not occur.
             */
            assert(thedesc.colStatus( col ) == Desc::P_ON_LOWER
               || thedesc.colStatus( col ) == Desc::P_FIXED);
         }
      }
   }

#ifndef NDEBUG
   thedesc.dump();

   // Check that we covered all nonbasic rows - the remaining should be basic.
   for (; row < theLP->nRows(); row++)
   {
      if ( ! theLP->isBasic( thedesc.rowStatus( row )))
         break;
   }
   assert( row == theLP->nRows() );

#endif // NDEBUG

   os << "ENDATA" << std::endl;
}


void SPxBasis::change(
   int i,
   SPxId& id,
   const SVector* enterVec,
   const SSVector* eta)
{
   METHOD( "SPxBasis::change()" );

   assert(matrixIsSetup);
   assert(!id.isValid() || (enterVec != 0));
   assert(factor != 0);

   lastidx = i;
   lastin  = id;

   if (id.isValid() && i >= 0)
   {
      assert(enterVec != 0);

      nzCount      = nzCount - matrix[i]->size() + enterVec->size();
      matrix[i]    = enterVec;
      lastout      = theBaseId[i];
      theBaseId[i] = id;

      ++iterCount;
      ++updateCount;

      // never factorize? Just do it !
      if (!factorized)
         factorize();
      // relative fill too high ?
      else if (Real(factor->memory()) > lastFill * Real(nzCount))
      {
         MSG_INFO3( spxout << "IBASIS04 fill factor triggers refactorization"
                              << " memory= " << factor->memory()
                              << " nzCount= " << nzCount
                              << " lastFill= " << lastFill
                              << std::endl; )

         factorize();
      }
      // absolute fill too high ?
      else if (nzCount > lastNzCount)
      {
         MSG_INFO3( spxout << "IBASIS05 nonzero factor triggers refactorization"
                              << " nzCount= " << nzCount
                              << " lastNzCount= " << lastNzCount
                              << " nonzeroFactor= " << nonzeroFactor
                              << std::endl; )
         factorize();
      }
      // too many updates ?
      else if (updateCount >= maxUpdates)
      {
         MSG_INFO3( spxout << "IBASIS06 update count triggers refactorization"
                              << " updateCount= " << updateCount
                              << " maxUpdates= " << maxUpdates
                              << std::endl; )
         factorize();
      }
      else
      {
         try
         {
            factor->change(i, *enterVec, eta);
         }
         catch( SPxException E )
         {
            MSG_INFO3( spxout << "IBASIS13 problems updating factorization; refactorizing basis"
               << std::endl; )

            // singularity was detected in update; we refactorize
            invalidate();
            factorize();

            // if factorize() detects singularity, an exception is thrown, hence at this point we have a regular basis
            // and can try the update again
            assert(status() >= SPxBasis::REGULAR);
            try
            {
               factor->change(i, *enterVec, eta);
            }
            // with a freshly factorized, regular basis, the update is unlikely to fail; if this happens nevertheless,
            // we have to invalidate the basis to have the statuses correct
            catch( SPxException F )
            {
               MSG_INFO3( spxout << "IBASIS14 problems updating factorization; invalidating basis"
                  << std::endl; )
               invalidate();
               throw F;
            }
         }

         assert(minStab > 0.0);

         if (factor->status() != SLinSolver::OK || factor->stability() < minStab)
         {
            MSG_INFO3( spxout << "IBASIS07 stability triggers refactorization"
                                 << " stability= " << factor->stability()
                                 << " minStab= " << minStab
                                 << std::endl; )
            factorize();
         }
      }
   }
   else
      lastout = id;
}

void SPxBasis::factorize()
{
   METHOD( "SPxBasis::factorize()" );

   assert(factor != 0);

   if (!matrixIsSetup)
      loadDesc(thedesc);

   assert(matrixIsSetup);

   updateCount = 0;
   
   switch(factor->load(matrix.get_ptr(), matrix.size()))
   {
   case SLinSolver::OK :
      if (status() == SINGULAR)
         setStatus(REGULAR);

      minStab = factor->stability();

      // This seems allways be about 1e-7 
      if (minStab > 1e-4)
         minStab *= 0.001;
      if (minStab > 1e-5)
         minStab *= 0.01;
      if (minStab > 1e-6)
         minStab *= 0.1;
      break;
   case SLinSolver::SINGULAR :
      setStatus(SINGULAR);
      break;
   default :
      MSG_ERROR( spxout << "EBASIS08 error: unknown status of factorization.\n"; )
      throw SPxInternalCodeException("XBASIS01 This should never happen.");
      // factorized = false;
   }

   lastMem    = factor->memory();
   lastFill   = fillFactor * Real(factor->memory()) / Real(nzCount > 0 ? nzCount : 1);
   lastNzCount = int(nonzeroFactor * Real(nzCount > 0 ? nzCount : 1));
   factorized = true;
}

Vector& SPxBasis::multWithBase(Vector& x) const
{
   METHOD( "SPxBasis::multWithBase()" );
   assert(status() > SINGULAR);
   assert(theLP->dim() == x.dim());

   int i;
   DVector tmp(x);

   if (!matrixIsSetup)
      (const_cast<SPxBasis*>(this))->loadDesc(thedesc);

   assert( matrixIsSetup );

   for (i = x.dim() - 1; i >= 0; --i)
      x[i] = *(matrix[i]) * tmp;

   return x;
}

Vector& SPxBasis::multBaseWith(Vector& x) const
{
   METHOD( "SPxBasis::multBaseWith()" );
   assert(status() > SINGULAR);
   assert(theLP->dim() == x.dim());

   int i;
   DVector tmp(x);

   if (!matrixIsSetup)
      (const_cast<SPxBasis*>(this))->loadDesc(thedesc);

   assert( matrixIsSetup );

   x.clear();
   for (i = x.dim() - 1; i >= 0; --i)
   {
      if (tmp[i])
         x.multAdd(tmp[i], *(matrix[i]));
   }

   return x;
}

void SPxBasis::dump()
{
   METHOD( "SPxBasis::dump()" );
   assert(status() > NO_PROBLEM);
   assert(theLP != 0);
   assert(thedesc.nRows() == theLP->nRows());
   assert(thedesc.nCols() == theLP->nCols());
   assert(theLP->dim() == matrix.size());

   int i, basesize;

   // Dump regardless of the verbosity level if this method is called.
   const SPxOut::Verbosity tmp_verbosity = spxout.getVerbosity();
   spxout.setVerbosity( SPxOut::ERROR );

   spxout << "DBASIS09 Basis entries:" << std::endl;
   basesize = 0;
   for (i = 0; i < theLP->nRows(); ++i)
   {
      if (theLP->isBasic(thedesc.rowStatus(i)))
      {
         SPxRowId id = theLP->SPxLP::rId(i);
         spxout << "DBASIS10 \tR" << theLP->number(id);
         basesize++;
         if(basesize % 8 == 0)
            spxout << std::endl;
      }
   }

   for (i = 0; i < theLP->nCols(); ++i)
   {
      if (theLP->isBasic(thedesc.colStatus(i)))
      {
         SPxColId id = theLP->SPxLP::cId(i);
         spxout << "DBASIS11 \tC" << theLP->number(id);
         basesize++;
         if(basesize % 8 == 0)
            spxout << std::endl;
      }
   }
   spxout << std::endl;

   assert(basesize == matrix.size());
   spxout.setVerbosity( tmp_verbosity );
}


#ifndef NO_CONSISTENCY_CHECKS
bool SPxBasis::isConsistent() const
{
   METHOD( "SPxBasis::isConsistent()" );
   int primals = 0;
   int i;

   if (status() > NO_PROBLEM)
   {
      if (theLP == 0)
         return MSGinconsistent("SPxBasis");

      if (theBaseId.size() != theLP->dim() || matrix.size() != theLP->dim())
         return MSGinconsistent("SPxBasis");

      if (thedesc.nCols() != theLP->nCols() || thedesc.nRows() != theLP->nRows())
         return MSGinconsistent("SPxBasis");

      for (i = 0; i < thedesc.nRows(); ++i)
      {
         if (thedesc.rowStatus(i) >= 0)
         {
            if (thedesc.rowStatus(i) != dualRowStatus(i))
               return MSGinconsistent("SPxBasis");
         }
         else
            ++primals;
      }
      
      for (i = 0; i < thedesc.nCols(); ++i)
      {
         if (thedesc.colStatus(i) >= 0)
         {
            if (thedesc.colStatus(i) != dualColStatus(i))
               return MSGinconsistent("SPxBasis");
         }
         else
            ++primals;
      }
      if (primals != thedesc.nCols())
         return MSGinconsistent("SPxBasis");
   }
   return thedesc.isConsistent() && theBaseId.isConsistent() 
      && matrix.isConsistent() && factor->isConsistent();
}
#endif // CONSISTENCY_CHECKS

SPxBasis::SPxBasis()
   : theLP (0)
   , matrixIsSetup (false)
   , factor (0)
   , factorized (false)
   , maxUpdates (200) 
   , nonzeroFactor(10.0)
   , fillFactor(5.0)
   , iterCount (0)
   , nzCount (1)
   , minStab(0.0)
   , thestatus (NO_PROBLEM)
   , freeSlinSolver(false)
{
   METHOD( "SPxBasis::SPxBasis()" );

   // info: is not consistent at this moment, e.g. because theLP == 0
}

/**@warning Do not change the LP object.
 *  Only pointer to that object is copied.
 *  Hint: no problem, we use this function for copy
 *   constructor of SPxSolver
 */
SPxBasis::SPxBasis(const SPxBasis& old)
   : theLP(old.theLP)
   , theBaseId(old.theBaseId)
   , matrix(old.matrix)
   , matrixIsSetup(old.matrixIsSetup)
   , factor(old.factor)
   , factorized(old.factorized)
   , maxUpdates(old.maxUpdates)
   , nonzeroFactor(old.nonzeroFactor)
   , fillFactor(old.fillFactor)
   , iterCount(old.iterCount)
   , nzCount(old.nzCount)
   , lastFill(old.lastFill)
   , lastNzCount(old.lastNzCount)
   , lastin(old.lastin)
   , lastout(old.lastout)
   , lastidx(old.lastidx)
   , minStab(old.minStab)
   , thestatus(old.thestatus)
   , thedesc(old.thedesc)
{
   METHOD( "SPxBasis::SPxBasis()" );

   this->factor = old.factor->clone();
   freeSlinSolver = true;

   assert(SPxBasis::isConsistent());
}

SPxBasis::~SPxBasis()
{
   METHOD( "SPxBasis::~SPxBasis()" );

   assert(!freeSlinSolver || factor != 0);

   if(freeSlinSolver)
   {
      delete factor;
      factor = 0;
   }
}


/**@warning  Note that we do not create a deep copy of the corresponding SPxSolver object.
 *  Only the reference to that object is copied.
 */
SPxBasis& SPxBasis::operator=(const SPxBasis& rhs)
{
   METHOD( "SPxBasis::operator=()" );

   assert(!freeSlinSolver || factor != 0);

   if (this != &rhs)
   {
      theLP         = rhs.theLP;
      theBaseId     = rhs.theBaseId;
      matrix        = rhs.matrix;
      matrixIsSetup = rhs.matrixIsSetup;

      if(freeSlinSolver)
      {
         delete factor;
         factor = 0;
      }
      factor = rhs.factor->clone();
      freeSlinSolver = true;
      
      factorized    = rhs.factorized;
      maxUpdates    = rhs.maxUpdates;
      nonzeroFactor = rhs.nonzeroFactor;
      fillFactor    = rhs.fillFactor;
      iterCount     = rhs.iterCount;
      nzCount       = rhs.nzCount;
      lastFill      = rhs.lastFill;
      lastNzCount   = rhs.lastNzCount;
      lastin        = rhs.lastin;
      lastout       = rhs.lastout;
      lastidx       = rhs.lastidx;
      minStab       = rhs.minStab;
      thestatus     = rhs.thestatus;
      thedesc       = rhs.thedesc;

      assert(SPxBasis::isConsistent());
   }

   return *this;
}


//
// Auxiliary functions.
//

// Pretty-printing of basis status.
std::ostream& operator<<( std::ostream& os,
                          const SPxBasis::SPxStatus& status )
{
   switch ( status )
      {
      case SPxBasis::NO_PROBLEM:
         os << "NO_PROBLEM";
         break;
      case SPxBasis::SINGULAR:
         os << "SINGULAR";
         break;
      case SPxBasis::REGULAR:
         os << "REGULAR";
         break;
      case SPxBasis::DUAL:
         os << "DUAL";
         break;
      case SPxBasis::PRIMAL:
         os << "PRIMAL";
         break;
      case SPxBasis::OPTIMAL:
         os << "OPTIMAL";
         break;
      case SPxBasis::UNBOUNDED:
         os << "UNBOUNDED";
         break;
      case SPxBasis::INFEASIBLE:
         os << "INFEASIBLE";
         break;
      default:
         os << "?other?";
         break;
      }
   return os;
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
