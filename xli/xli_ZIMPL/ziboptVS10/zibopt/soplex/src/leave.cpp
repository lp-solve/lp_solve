/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SsoPlex --- the Sequential object-oriented simPlex.                  */
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
#pragma ident "@(#) $Id: leave.cpp,v 1.58 2010/09/16 17:45:02 bzfgleix Exp $"

//#define DEBUGGING 1

/* Updating the Basis for Leaving Variables
 */
#include <assert.h>
#include <stdio.h>

#include "spxdefines.h"
#include "spxsolver.h"
#include "spxratiotester.h"
#include "spxout.h"
#include "exceptions.h"

namespace soplex
{
static const Real reject_leave_tol = 1e-10; // = LOWSTAB as defined in spxfastrt.cpp

/*
    Vector |fTest| gives the feasibility test of all basic variables. For its
    computation |fVec|, |theUBbound| and |theLBbound| must be setup correctly.
    Values of |fTest| $<0$ represent infeasible variables, which are eligible
    for leaving the basis in the simplex loop.
 */
void SPxSolver::computeFtest()
{
   METHOD( "SPxSolver::computeFtest()" );

   assert(type() == LEAVE);

   for(int i = 0; i < dim(); ++i)
   {
      theCoTest[i] = ((*theFvec)[i] > theUBbound[i])
         ? theUBbound[i] - (*theFvec)[i]
         : (*theFvec)[i] - theLBbound[i];
   }
}

void SPxSolver::updateFtest()
{
   METHOD( "SPxSolver::updateFtest()" );
   const IdxSet& idx = theFvec->idx();
   Vector& ftest = theCoTest;      // |== fTest()|
   assert(&ftest == &fTest());

   assert(type() == LEAVE);
   for (int j = idx.size() - 1; j >= 0; --j)
   {
      int i = idx.index(j);

      ftest[i] = ((*theFvec)[i] > theUBbound[i])
         ? theUBbound[i] - (*theFvec)[i]
         : (*theFvec)[i] - theLBbound[i];
   }
}


/* compute statistics on leaving variable 
   Compute a set of statistical values on the variable selected for leaving the
   basis.
 */
void SPxSolver::getLeaveVals(
   int leaveIdx,
   SPxBasis::Desc::Status& leaveStat,
   SPxId& leaveId,
   Real& leaveMax,
   Real& leavebound,
   int& leaveNum)
{
   METHOD( "SPxSolver::getLeaveVals()" );
   SPxBasis::Desc& ds = desc();
   leaveId = baseId(leaveIdx);

   if (leaveId.isSPxRowId())
   {
      leaveNum = number(SPxRowId(leaveId));
      leaveStat = ds.rowStatus(leaveNum);

      assert(isBasic(leaveStat));
      switch (leaveStat)
      {
      case SPxBasis::Desc::P_ON_UPPER :
         assert( rep() == ROW );
         ds.rowStatus(leaveNum) = dualRowStatus(leaveNum);
         leavebound = 0;
         leaveMax = -infinity;
         break;
      case SPxBasis::Desc::P_ON_LOWER :
         assert( rep() == ROW );
         ds.rowStatus(leaveNum) = dualRowStatus(leaveNum);
         leavebound = 0;
         leaveMax = infinity;
         break;
      case SPxBasis::Desc::P_FREE :
         assert( rep() == ROW );
         throw SPxInternalCodeException("XLEAVE01 This should never happen.");
      case SPxBasis::Desc::D_FREE :
         assert( rep() == COLUMN );
         ds.rowStatus(leaveNum) = SPxBasis::Desc::P_FIXED;
         assert(lhs(leaveNum) == rhs(leaveNum));
         leavebound = -rhs(leaveNum);
         if ((*theFvec)[leaveIdx] < theLBbound[leaveIdx])
            leaveMax = infinity;
         else
            leaveMax = -infinity;
         break;
      case SPxBasis::Desc::D_ON_LOWER :
         assert( rep() == COLUMN );
         ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
         leavebound = -rhs(leaveNum);                // slack !!
         leaveMax = infinity;
         break;
      case SPxBasis::Desc::D_ON_UPPER :
         assert( rep() == COLUMN );
         ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
         leavebound = -lhs(leaveNum);                // slack !!
         leaveMax = -infinity;
         break;
      case SPxBasis::Desc::D_ON_BOTH :
         assert( rep() == COLUMN );
         if ((*theFvec)[leaveIdx] > theLBbound[leaveIdx])
         {
            ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
            theLRbound[leaveNum] = -infinity;
            leavebound = -lhs(leaveNum);            // slack !!
            leaveMax = -infinity;
         }
         else
         {
            ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
            theURbound[leaveNum] = infinity;
            leavebound = -rhs(leaveNum);            // slack !!
            leaveMax = infinity;
         }
         break;

      default:
         throw SPxInternalCodeException("XLEAVE02 This should never happen.");
      }
      MSG_DEBUG( spxout << "DLEAVE51 SPxSolver::getLeaveVals() : row " << leaveNum
                        << ": " << leaveStat
                        << " -> " << ds.rowStatus(leaveNum)
                        << std::endl; )
   }

   else
   {
      assert(leaveId.isSPxColId());
      leaveNum = number(SPxColId(leaveId));
      leaveStat = ds.colStatus(leaveNum);

      assert(isBasic(leaveStat));
      switch (leaveStat)
      {
      case SPxBasis::Desc::P_ON_UPPER :
         assert( rep() == ROW );
         ds.colStatus(leaveNum) = dualColStatus(leaveNum);
         leavebound = 0;
         leaveMax = -infinity;
         break;
      case SPxBasis::Desc::P_ON_LOWER :
         assert( rep() == ROW );
         ds.colStatus(leaveNum) = dualColStatus(leaveNum);
         leavebound = 0;
         leaveMax = infinity;
         break;
      case SPxBasis::Desc::P_FREE :
         assert( rep() == ROW );
         ds.colStatus(leaveNum) = dualColStatus(leaveNum);
         if ((*theFvec)[leaveIdx] < theLBbound[leaveIdx])
         {
            leavebound = theLBbound[leaveIdx];
            leaveMax = -infinity;
         }
         else
         {
            leavebound = theUBbound[leaveIdx];
            leaveMax = infinity;
         }
         break;

      case SPxBasis::Desc::D_FREE :
         assert( rep() == COLUMN );
         assert(SPxLP::upper(leaveNum) == SPxLP::lower(leaveNum));
         ds.colStatus(leaveNum) = SPxBasis::Desc::P_FIXED;
         leavebound = SPxLP::upper(leaveNum);
         if ((*theFvec)[leaveIdx] < theLBbound[leaveIdx])
            leaveMax = infinity;
         else
            leaveMax = -infinity;
         break;
      case SPxBasis::Desc::D_ON_LOWER :
         assert( rep() == COLUMN );
         ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
         leavebound = SPxLP::upper(leaveNum);
         leaveMax = -infinity;
         break;
      case SPxBasis::Desc::D_ON_UPPER :
         assert( rep() == COLUMN );
         ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
         leavebound = SPxLP::lower(leaveNum);
         leaveMax = infinity;
         break;
      case SPxBasis::Desc::D_ON_BOTH :
         assert( rep() == COLUMN );
         if ((*theFvec)[leaveIdx] > theUBbound[leaveIdx])
         {
            leaveMax = -infinity;
            leavebound = SPxLP::upper(leaveNum);
            theLCbound[leaveNum] = -infinity;
            ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
         }
         else
         {
            leaveMax = infinity;
            leavebound = SPxLP::lower(leaveNum);
            theUCbound[leaveNum] = infinity;
            ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
         }
         break;
      default:
         throw SPxInternalCodeException("XLEAVE03 This should never happen.");
      }
      MSG_DEBUG( spxout << "DLEAVE52 SPxSolver::getLeaveVals() : col " << leaveNum
                        << ": " << leaveStat
                        << " -> " << ds.colStatus(leaveNum)
                        << std::endl; )
   }
}

void SPxSolver::getLeaveVals2(
   Real leaveMax,
   SPxId enterId,
   Real& enterBound,
   Real& newUBbound,
   Real& newLBbound,
   Real& newCoPrhs
)
{
   METHOD( "SPxSolver::getLeaveVals2()" );
   SPxBasis::Desc& ds = desc();

   enterBound = 0;
   if (enterId.isSPxRowId())
   {
      int idx = number(SPxRowId(enterId));
      SPxBasis::Desc::Status enterStat = ds.rowStatus(idx);

      switch (enterStat)
      {
      case SPxBasis::Desc::D_FREE :
         assert(rep() == ROW);
         if (thePvec->delta()[idx] * leaveMax < 0)
            newCoPrhs = theLRbound[idx];
         else
            newCoPrhs = theURbound[idx];
         newUBbound = infinity;
         newLBbound = -infinity;
         ds.rowStatus(idx) = SPxBasis::Desc::P_FIXED;
         break;
      case SPxBasis::Desc::D_ON_UPPER :
         assert(rep() == ROW);
         newUBbound = 0;
         newLBbound = -infinity;
         ds.rowStatus(idx) = SPxBasis::Desc::P_ON_LOWER;
         newCoPrhs = theLRbound[idx];
         break;
      case SPxBasis::Desc::D_ON_LOWER :
         assert(rep() == ROW);
         newUBbound = infinity;
         newLBbound = 0;
         ds.rowStatus(idx) = SPxBasis::Desc::P_ON_UPPER;
         newCoPrhs = theURbound[idx];
         break;
      case SPxBasis::Desc::D_ON_BOTH :
         assert(rep() == ROW);
         if (leaveMax * thePvec->delta()[idx] < 0)
         {
            newUBbound = 0;
            newLBbound = -infinity;
            ds.rowStatus(idx) = SPxBasis::Desc::P_ON_LOWER;
            newCoPrhs = theLRbound[idx];
         }
         else
         {
            newUBbound = infinity;
            newLBbound = 0;
            ds.rowStatus(idx) = SPxBasis::Desc::P_ON_UPPER;
            newCoPrhs = theURbound[idx];
         }
         break;

      case SPxBasis::Desc::P_ON_UPPER :
         assert(rep() == COLUMN);
         ds.rowStatus(idx) = dualRowStatus(idx);
         if (lhs(idx) > -infinity)
            theURbound[idx] = theLRbound[idx];
         newCoPrhs = theLRbound[idx];        // slack !!
         newUBbound = -lhs(idx);
         newLBbound = -rhs(idx);
         enterBound = -rhs(idx);
         break;
      case SPxBasis::Desc::P_ON_LOWER :
         assert(rep() == COLUMN);
         ds.rowStatus(idx) = dualRowStatus(idx);
         if (rhs(idx) < infinity)
            theLRbound[idx] = theURbound[idx];
         newCoPrhs = theURbound[idx];        // slack !!
         newLBbound = -rhs(idx);
         newUBbound = -lhs(idx);
         enterBound = -lhs(idx);
         break;
      case SPxBasis::Desc::P_FREE :
         assert(rep() == COLUMN);
#if 1
         throw SPxInternalCodeException("XLEAVE04 This should never happen.");
#else
         MSG_ERROR( spxout << "ELEAVE53 ERROR: not yet debugged!" << std::endl; )
         ds.rowStatus(idx) = dualRowStatus(idx);
         newCoPrhs = theURbound[idx];        // slack !!
         newUBbound = infinity;
         newLBbound = -infinity;
         enterBound = 0;
#endif
         break;
      case SPxBasis::Desc::P_FIXED :
         assert(rep() == COLUMN);
         MSG_ERROR( spxout << "ELEAVE54 "
                           << "ERROR! Tried to put a fixed row variable into the basis: "
                           << "idx="   << idx
                           << ", lhs=" << lhs(idx)
                           << ", rhs=" << rhs(idx) << std::endl; )
         throw SPxInternalCodeException("XLEAVE05 This should never happen.");

      default:
         throw SPxInternalCodeException("XLEAVE06 This should never happen.");
      }
      MSG_DEBUG( spxout << "DLEAVE55 SPxSolver::getLeaveVals2(): row " << idx
                        << ": " << enterStat
                        << " -> " << ds.rowStatus(idx)
                        << std::endl; )
   }

   else
   {
      assert(enterId.isSPxColId());
      int idx = number(SPxColId(enterId));
      SPxBasis::Desc::Status enterStat = ds.colStatus(idx);

      switch (enterStat)
      {
      case SPxBasis::Desc::D_ON_UPPER :
         assert(rep() == ROW);
         newUBbound = 0;
         newLBbound = -infinity;
         ds.colStatus(idx) = SPxBasis::Desc::P_ON_LOWER;
         newCoPrhs = theLCbound[idx];
         break;
      case SPxBasis::Desc::D_ON_LOWER :
         assert(rep() == ROW);
         newUBbound = infinity;
         newLBbound = 0;
         ds.colStatus(idx) = SPxBasis::Desc::P_ON_UPPER;
         newCoPrhs = theUCbound[idx];
         break;
      case SPxBasis::Desc::D_FREE :
         assert(rep() == ROW);
         newUBbound = infinity;
         newLBbound = -infinity;
         newCoPrhs = theLCbound[idx];
         ds.colStatus(idx) = SPxBasis::Desc::P_FIXED;
         break;
      case SPxBasis::Desc::D_ON_BOTH :
         assert(rep() == ROW);
         if (leaveMax * theCoPvec->delta()[idx] < 0)
         {
            newUBbound = 0;
            newLBbound = -infinity;
            ds.colStatus(idx) = SPxBasis::Desc::P_ON_LOWER;
            newCoPrhs = theLCbound[idx];
         }
         else
         {
            newUBbound = infinity;
            newLBbound = 0;
            ds.colStatus(idx) = SPxBasis::Desc::P_ON_UPPER;
            newCoPrhs = theUCbound[idx];
         }
         break;

      case SPxBasis::Desc::P_ON_UPPER :
         assert(rep() == COLUMN);
         ds.colStatus(idx) = dualColStatus(idx);
         if (SPxLP::lower(idx) > -infinity)
            theLCbound[idx] = theUCbound[idx];
         newCoPrhs = theUCbound[idx];
         newUBbound = SPxLP::upper(idx);
         newLBbound = SPxLP::lower(idx);
         enterBound = SPxLP::upper(idx);
         break;
      case SPxBasis::Desc::P_ON_LOWER :
         assert(rep() == COLUMN);
         ds.colStatus(idx) = dualColStatus(idx);
         if (SPxLP::upper(idx) < infinity)
            theUCbound[idx] = theLCbound[idx];
         newCoPrhs = theLCbound[idx];
         newUBbound = SPxLP::upper(idx);
         newLBbound = SPxLP::lower(idx);
         enterBound = SPxLP::lower(idx);
         break;
      case SPxBasis::Desc::P_FREE :
         assert(rep() == COLUMN);
         ds.colStatus(idx) = dualColStatus(idx);
         if (thePvec->delta()[idx] * leaveMax > 0)
            newCoPrhs = theUCbound[idx];
         else
            newCoPrhs = theLCbound[idx];
         newUBbound = SPxLP::upper(idx);
         newLBbound = SPxLP::lower(idx);
         enterBound = 0;
         break;
      case SPxBasis::Desc::P_FIXED :
         assert(rep() == COLUMN);
         MSG_ERROR( spxout << "ELEAVE56 "
                           << "ERROR! Tried to put a fixed column variable into the basis. "
                           << "idx="     << idx
                           << ", lower=" << lower(idx)
                           << ", upper=" << upper(idx) << std::endl; )
         throw SPxInternalCodeException("XLEAVE07 This should never happen.");
      default:
         throw SPxInternalCodeException("XLEAVE08 This should never happen.");
      }

      MSG_DEBUG( spxout << "DLEAVE57 col " << idx
                        << ": " << enterStat
                        << " -> " << ds.colStatus(idx)
                        << std::endl; )
   }

}

void SPxSolver::rejectLeave(
   int leaveNum,
   SPxId leaveId,
   SPxBasis::Desc::Status leaveStat,
   const SVector* //newVec
)
{
   METHOD( "SPxSolver::rejectLeave()" );
   SPxBasis::Desc& ds = desc();
   if (leaveId.isSPxRowId())
   {
      MSG_DEBUG( spxout << "DLEAVE58 rejectLeave()  : row " << leaveNum
                        << ": " << ds.rowStatus(leaveNum)
                        << " -> " << leaveStat << std::endl; )

      if (leaveStat == SPxBasis::Desc::D_ON_BOTH)
      {
         if (ds.rowStatus(leaveNum) == SPxBasis::Desc::P_ON_LOWER)
            theLRbound[leaveNum] = theURbound[leaveNum];
         else
            theURbound[leaveNum] = theLRbound[leaveNum];
      }
      ds.rowStatus(leaveNum) = leaveStat;
   }
   else
   {
      MSG_DEBUG( spxout << "DLEAVE59 rejectLeave()  : col " << leaveNum
                        << ": " << ds.colStatus(leaveNum)
                        << " -> " << leaveStat << std::endl; )

      if (leaveStat == SPxBasis::Desc::D_ON_BOTH)
      {
         if (ds.colStatus(leaveNum) == SPxBasis::Desc::P_ON_UPPER)
            theLCbound[leaveNum] = theUCbound[leaveNum];
         else
            theUCbound[leaveNum] = theLCbound[leaveNum];
      }
      ds.colStatus(leaveNum) = leaveStat;
   }
}


bool SPxSolver::leave(int leaveIdx)
{
   METHOD( "SPxSolver::leave()" );
   assert(leaveIdx < dim() && leaveIdx >= 0);
   assert(type() == LEAVE);
   assert(initialized);

   bool instable = instableLeave;
   assert(!instable || instableLeaveNum >= 0);

   /*
       Before performing the actual basis update, we must determine, how this
       is to be accomplished.
    */
   if (theCoPvec->delta().isSetup() && theCoPvec->delta().size() == 0)
   {
      coSolve(theCoPvec->delta(), unitVecs[leaveIdx]);
   }
#if ENABLE_ADDITIONAL_CHECKS
   else
   {
      SSVector tmp(dim(), epsilon());
      tmp.clear();
      coSolve(tmp, unitVecs[leaveIdx]);
      tmp -= theCoPvec->delta();
      if (tmp.length() > delta()) {
         // This happens very frequently and does usually not hurt, so print
         // these warnings only with verbose level INFO2 and higher.
         MSG_INFO2( spxout << "WLEAVE60 iteration=" << basis().iteration() 
                              << ": coPvec.delta error = " << tmp.length() 
                              << std::endl; )
      }
   }
#endif  // ENABLE_ADDITIONAL_CHECKS

   setupPupdate();

   assert(thePvec->isConsistent());
   assert(theCoPvec->isConsistent());

   SPxBasis::Desc::Status leaveStat;      // status of leaving var
   SPxId leaveId;        // id of leaving var
   Real leaveMax;       // maximium lambda of leaving var
   Real leavebound;     // current fVec value of leaving var
   int  leaveNum;       // number of leaveId in bounds

   getLeaveVals(leaveIdx, leaveStat, leaveId, leaveMax, leavebound, leaveNum);

   if (m_numCycle > m_maxCycle)
   {
      if (leaveMax > 0)
         perturbMaxLeave();
      else
         perturbMinLeave();
      //@ m_numCycle /= 2;
   }
   //@ testBounds();

   for(;;)
   {
      Real enterVal = leaveMax;
      SPxId enterId = theratiotester->selectEnter(enterVal);

      instableLeaveNum = -1;
      instableLeave = false;

      /*
          No variable could be selected to enter the basis and even the leaving
          variable is unbounded --- this is a failure.
       */
      if (!enterId.isValid())
      {
         SPxId none;

         change(leaveIdx, none, 0);
         /* the following line originally was below in "rejecting leave" case;
            we need it in the unbounded/infeasible case, too, to have the 
            correct basis size */

         rejectLeave(leaveNum, leaveId, leaveStat);
            
         if (enterVal != leaveMax)
         {
            MSG_DEBUG( spxout << "DLEAVE61 rejecting leave A (leaveIdx=" << leaveIdx
                              << ", theCoTest=" << theCoTest[leaveIdx] << ")" 
                              << std::endl; )

            /* In the LEAVE algorithm, when for a selected leaving variable we find only
               an instable entering variable, then the basis change is not conducted.
               Instead, we save the leaving variable's index in instableLeaveNum and scale
               theCoTest[leaveIdx] down by some factor, hoping to find a different leaving
               variable with a stable entering variable.
               If this fails, however, and no more leaving variable is found, we have to
               perform the instable basis change using instableLeaveNum. In this (and only
               in this) case, the flag instableLeave is set to true.
               
               enterVal != leaveMax is the case that selectEnter has found only an instable entering
               variable. We store this leaving variable for later -- if we are not already in the
               instable case: then we continue and conclude unboundness/infeasiblity */
            if (!instable)
            {
               instableLeaveNum = leaveIdx;

               // Note: These changes do not survive a refactorization
               theCoTest[leaveIdx] *= 0.01;
               //            theCoTest[leaveIdx] -= 2 * delta();

               return true;
            }
         }
         if (lastUpdate() > 1)
         {
            MSG_INFO3( spxout << "ILEAVE01 factorization triggered in "
                                 << "leave() for feasibility test" << std::endl; )
            factorize();
            return leave(leaveIdx);
         }
         MSG_INFO3( spxout << "ILEAVE02 unboundness/infeasiblity found "
                              << "in leave()" << std::endl; )

         if (rep() != COLUMN)
            setBasisStatus(SPxBasis::UNBOUNDED);
         else
         {
            int sign;
            int i;

            dualFarkas.clear();
            dualFarkas.setMax(coPvec().delta().size());
            sign = (enterVal > 0 ? -1 : +1);
            for( i = 0; i < coPvec().delta().size(); ++i )
               dualFarkas.add(coPvec().delta().index(i), sign * coPvec().delta().value(i));

            setBasisStatus(SPxBasis::INFEASIBLE);
         }
         return false;
      }
      else 
      {
         /*
           If an entering variable has been found, a regular basis update is to
           be performed.
         */
         if (enterId != baseId(leaveIdx))
         {
            const SVector& newVector = *enterVector(enterId);

            // update feasibility vectors
            if (solveVector2)
               SPxBasis::solve4update (theFvec->delta(), *solveVector2,
                  newVector, *solveVector2rhs);
            else
               SPxBasis::solve4update (theFvec->delta(), newVector);

#if ENABLE_ADDITIONAL_CHECKS
            {
               SSVector tmp(dim(), epsilon());
               SPxBasis::solve(tmp, newVector);
               tmp -= fVec().delta();
               if (tmp.length() > delta()) {
                  // This happens very frequently and does usually not hurt, so print
                  // these warnings only with verbose level INFO2 and higher.
                  MSG_INFO2( spxout << "WLEAVE62\t(" << tmp.length() << ")\n"; )
                     }
            }
#endif  // ENABLE_ADDITIONAL_CHECKS


            if (fabs(theFvec->delta()[leaveIdx]) < reject_leave_tol)
            {
               if (instable)
               {
                  /* We are in the case that for all leaving variables only instable entering
                     variables were found: Thus, above we already accepted such an instable
                     entering variable. Now even this seems to be impossible, thus we conclude
                     unboundedness/infeasibility. */
                  MSG_INFO3( spxout << "ILEAVE03 unboundness/infeasiblity found "
                     << "in leave()" << std::endl; )

                  SPxId none;
                  change(leaveIdx, none, 0);
                  rejectLeave(leaveNum, leaveId, leaveStat);

                  if (rep() != COLUMN)
                     setBasisStatus(SPxBasis::UNBOUNDED);
                  else
                  {
                     int sign;
                     int i;

                     dualFarkas.clear();
                     dualFarkas.setMax(coPvec().delta().size());
                     sign = (enterVal > 0 ? -1 : +1);
                     for( i = 0; i < coPvec().delta().size(); ++i )
                        dualFarkas.add(coPvec().delta().index(i), sign * coPvec().delta().value(i));

                     setBasisStatus(SPxBasis::INFEASIBLE);
                  }

                  return false;
               }
               else
               {
                  SPxId none;
                  change(leaveIdx, none, 0);
                  theFvec->delta().clear();
                  rejectLeave(leaveNum, leaveId, leaveStat, &newVector);
                  MSG_DEBUG( spxout << "DLEAVE63 rejecting leave B (leaveIdx=" << leaveIdx
                     << ", theCoTest=" << theCoTest[leaveIdx] 
                     << ")" << std::endl; )

                  // factorize();

                  // Note: These changes do not survive a refactorization
                  theCoTest[leaveIdx] *= 0.01;

                  return true;
               }
            }

            //      process leaving variable
            if (leavebound > epsilon() || leavebound < -epsilon())
               theFrhs->multAdd(-leavebound, baseVec(leaveIdx));

            //      process entering variable
            Real enterBound;
            Real newUBbound;
            Real newLBbound;
            Real newCoPrhs;

            getLeaveVals2(leaveMax, enterId,
               enterBound, newUBbound, newLBbound, newCoPrhs);

            theUBbound[leaveIdx] = newUBbound;
            theLBbound[leaveIdx] = newLBbound;
            (*theCoPrhs)[leaveIdx] = newCoPrhs;

            if (enterBound > epsilon() || enterBound < -epsilon())
               theFrhs->multAdd(enterBound, newVector);

            // update pricing vectors
            theCoPvec->value() = enterVal;
            thePvec->value() = enterVal;
            if (enterVal > epsilon() || enterVal < -epsilon())
               doPupdate();

            // update feasibility vector
            theFvec->value() = -((*theFvec)[leaveIdx] - leavebound)
               / theFvec->delta()[leaveIdx];
            theFvec->update();
            (*theFvec)[leaveIdx] = enterBound - theFvec->value();
            updateFtest();

            //  change basis matrix
            change(leaveIdx, enterId, &newVector, &(theFvec->delta()));
         }

         /*
           No entering vector has been selected from the basis. However, if the
           shift amount for |coPvec| is bounded, we are in the case, that the
           entering variable is moved from one bound to its other, before any of
           the basis feasibility variables reaches their bound. This may only
           happen in primal/columnwise case with upper and lower bounds on
           variables.
         */
         else
         {
            assert(rep() == ROW);
            SPxBasis::Desc& ds = desc();

            SPxId none;
            change(leaveIdx, none, 0);

            if (leaveStat == SPxBasis::Desc::P_ON_UPPER)
            {
               if (leaveId.isSPxRowId())
               {
                  ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
                  (*theCoPrhs)[leaveIdx] = theLRbound[leaveNum];
               }
               else
               {
                  ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_LOWER;
                  (*theCoPrhs)[leaveIdx] = theLCbound[leaveNum];
               }
               theUBbound[leaveIdx] = 0;
               theLBbound[leaveIdx] = -infinity;
            }
            else
            {
               assert( leaveStat == SPxBasis::Desc::P_ON_LOWER );
               if (leaveId.isSPxRowId())
               {
                  ds.rowStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
                  (*theCoPrhs)[leaveIdx] = theURbound[leaveNum];
               }
               else
               {
                  ds.colStatus(leaveNum) = SPxBasis::Desc::P_ON_UPPER;
                  (*theCoPrhs)[leaveIdx] = theUCbound[leaveNum];
               }
               theUBbound[leaveIdx] = infinity;
               theLBbound[leaveIdx] = 0;
            }

            // update copricing vector
            theCoPvec->value() = enterVal;
            thePvec->value() = enterVal;
            if (enterVal > epsilon() || enterVal < -epsilon())
               doPupdate();

            // update feasibility vectors
            theFvec->value() = 0;
            theCoTest[leaveIdx] *= -1;
         }

         if ((leaveMax > delta() && enterVal <= delta()) || (leaveMax < -delta() && enterVal >= -delta()))
         {
            if ((theUBbound[leaveIdx] < infinity || theLBbound[leaveIdx] > -infinity)
               && leaveStat != SPxBasis::Desc::P_FREE
               && leaveStat != SPxBasis::Desc::D_FREE)
               m_numCycle++;
         }
         else
            m_numCycle /= 2;

#if ENABLE_ADDITIONAL_CHECKS
         {
            DVector tmp = fVec();
            multBaseWith(tmp);
            tmp -= fRhs();
            if (tmp.length() > delta())
            {
               // This happens very frequently and does usually not hurt, so print
               // these warnings only with verbose level INFO2 and higher.
               MSG_INFO2( spxout << "WLEAVE64\t" << basis().iteration()
                  << ": fVec error = " << tmp.length() << std::endl; )
                  SPxBasis::solve(tmp, fRhs());
               tmp -= fVec();
               MSG_INFO2( spxout << "WLEAVE65\t(" << tmp.length() << ")\n"; )
                  }
         }
#endif  // ENABLE_ADDITIONAL_CHECKS

         return true;
      }
   }
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
