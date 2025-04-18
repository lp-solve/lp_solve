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
#pragma ident "@(#) $Id: soplex.h,v 1.84 2010/10/01 18:20:15 bzfgleix Exp $"

/**@file  soplex.h
 * @brief preconfigured \ref soplex::SoPlex "SoPlex" LP-solver.
 */
#ifndef _SOPLEX_H_
#define _SOPLEX_H_

#include <assert.h>
#include <string>

#include "spxsolver.h"
#include "spxscaler.h"
#include "spxsimplifier.h"
#include "spxsteeppr.h"
#include "spxfastrt.h"
#include "spxweightst.h"
#include "slufactor.h"

namespace soplex
{

/**@class SoPlex
   @brief   Preconfigured SoPlex LP-solver.
   @ingroup Algo
*/
class SoPlex : public SPxLP
{
protected:

   //-------------------------
   //**@name Protected data */
   //@{
   //   SPxWeightST st;  ///< weight starter
   SLUFactor       m_slu;        ///< LU Factorisation
   SPxSolver       m_solver;     ///< solver
   SPxScaler*      m_preScaler;  ///< pre-scaler
   SPxScaler*      m_postScaler; ///< post-scaler
   SPxSimplifier*  m_simplifier; ///< simplifier
   bool            m_vanished;   ///< did the presolver solve the problem ?
   bool            m_freePreScaler;   ///< true iff m_preScaler should be freed inside of this object
   bool            m_freePostScaler;  ///< true iff m_postScaler should be freed inside of this object
   bool            m_freeSimplifier;  ///< true iff m_simplifier should be freed inside of this object
   //@}

public:

   //---------------------------------------
   //**@name Construction / destruction */
   //@{
   /// default construtor.
   explicit SoPlex(
      SPxSolver::Type           type = SPxSolver::LEAVE, 
      SPxSolver::Representation rep  = SPxSolver::COLUMN );
   virtual ~SoPlex();
   /// assignment operator.
   SoPlex& operator=(const SoPlex& rhs);
   /// copy constructor.
   SoPlex(const SoPlex&);
   //@}

   //---------------------------------------
   //**@name Access / modification */
   //@{
   /// set update type for factorization.
   virtual void setUtype(SLUFactor::UpdateType tp)
   {
      m_slu.setUtype(tp);
   }
   /// return current Pricing.
   inline SPxSolver::Pricing pricing() const
   {
      return m_solver.pricing();
   }
   /// set FULL or PARTIAL pricing.
   virtual void setPricing(SPxSolver::Pricing pr)
   {
      m_solver.setPricing(pr);
   }
   /// return current Type.
   inline SPxSolver::Type type() const
   {
      return m_solver.type();
   }
   /// return current basis representation.
   inline SPxSolver::Representation rep() const
   {
      return m_solver.rep();
   }
   /// set LEAVE or ENTER algorithm.
   virtual void setType(SPxSolver::Type tp)
   {
      m_solver.setType(tp);
   }
   /// set ROW or COLUMN representation.
   virtual void setRep (SPxSolver::Representation p_rep)
   {
      m_solver.setRep(p_rep);
   }
   /// setup prescaler to use. If \p destroy is true, \p scaler will be freed in destructor.
   virtual void setPreScaler(SPxScaler* scaler, const bool destroy = false);
   /// setup postscaler to use. If \p destroy is true, \p scaler will be freed in destructor.
   virtual void setPostScaler(SPxScaler* scaler, const bool destroy = false);
   /// setup simplifier to use. If \p destroy is true, \p simpli will be freed in destructor.
   virtual void setSimplifier(SPxSimplifier* simpli, const bool destroy = false);
   /// has a simplifier been set?
   inline bool has_simplifier() const
   {
      return m_simplifier != 0;
   }
   /// has a prescaler been set?
   inline bool has_prescaler() const
   {
      return m_preScaler != 0;
   }
   /// has a postscaler been set?
   inline bool has_postscaler() const
   {
      return m_postScaler != 0;
   }
   /// setup pricer to use.
   virtual void setPricer(SPxPricer* pricer, const bool destroy = false)
   {
      m_solver.setPricer(pricer, destroy);
   }
   /// setup ratio-tester to use.
   virtual void setTester(SPxRatioTester* tester, const bool destroy = false)
   {
      m_solver.setTester(tester, destroy);
   }
   /// setup starting basis generator to use.
   virtual void setStarter(SPxStarter* starter, const bool destroy = false)
   {
      m_solver.setStarter(starter, destroy);
   }
   /// @throw SPxStatusException if simplifier loaded, this is not yet implemented
   /// set starting basis
   virtual void setBasis(SPxSolver::VarStatus rows[], SPxSolver::VarStatus cols[])
   {
      if (has_simplifier())
      {
         MSG_ERROR( spxout << "ESOLVR04 setting starting basis with presolving not yet implemented" << std::endl; )
            throw SPxStatusException("XSOLVR04 setting starting basis with presolving not yet implemented");
      }
      m_solver.setBasis(rows, cols);
   }
   /// set time limit.
   virtual void setTerminationTime(Real time = infinity)
   {
      m_solver.setTerminationTime(time);
   }
   /// return time limit.
   inline Real terminationTime() const
   {
      return m_solver.terminationTime();
   }
   /// set iteration limit.
   virtual void setTerminationIter(int iter = -1)
   {
      m_solver.setTerminationIter(iter);
   }
   /// return iteration limit.
   inline int terminationIter() const
   {
      return m_solver.terminationIter();
   }
   /// set objective limit.
   virtual void setTerminationValue(Real val = infinity)
   {
      m_solver.setTerminationValue(val);
   }
   /// return objective limit.
   inline Real terminationValue() const
   {
      return m_solver.terminationValue();
   }
   /// allowed bound violation for optimal solution.
   /** When no vector violates its bound by more than \f$\delta\f$,
    *  the basis is considered optimal.
    */
   virtual Real delta() const
   {
      return m_solver.delta();
   }
   /// set parameter \p delta.
   virtual void setDelta(Real d)
   {
      m_solver.setDelta(d);
   }
   //@}

   //---------------------------------------
   //**@name Solving and solution query */
   //@{
   /// @throw SPxStatusException if no problem loaded
   virtual SPxSolver::Status solve();
   ///
   virtual Real objValue() const;
   ///
   virtual SPxSolver::Status getPrimal(Vector& vector) const;
   ///
   virtual SPxSolver::Status getSlacks(Vector& vector) const;
   ///
   virtual SPxSolver::Status getDual(Vector& vector) const;
   ///
   virtual SPxSolver::Status getRedCost(Vector& vector) const;
 
   /// gets basis status for a single row.
   SPxSolver::VarStatus getBasisRowStatus(int row) const;

   /// gets basis status for a single column.
   SPxSolver::VarStatus getBasisColStatus(int col) const;

   /// get current basis, and return solver status.
   SPxSolver::Status getBasis(SPxSolver::VarStatus rows[], SPxSolver::VarStatus cols[]) const;

   /// @throw SPxStatusException if simplifier loaded, this is not yet
   /// implemented
   virtual SPxSolver::Status getDualfarkas(Vector& vector) const;

   /// get violation of constraints.
   virtual void qualConstraintViolation(Real& maxviol, Real& sumviol) const;
   /// get violations of bounds.
   virtual void qualBoundViolation(Real& maxviol, Real& sumviol) const;
#if 0
   /// get the residuum |Ax-b|.
   virtual void qualSlackViolation(Real& maxviol, Real& sumviol) const;
   /// get violation of optimality criterion.
   virtual void qualRedCostViolation(Real& maxviol, Real& sumviol) const;
#endif
   /// time spent in factorizations
   virtual Real getFactorTime() const
   {
      return m_vanished ? REAL(0.0) : m_slu.getFactorTime();
   }
   /// number of factorizations performed
   virtual int getFactorCount() const
   {
      return m_vanished ? 0 : m_slu.getFactorCount();
   }
   /// time spent in solves
   virtual Real getSolveTime() const
   {
      return m_vanished ? REAL(0.0) : m_slu.getSolveTime();
   }
   /// number of solves performed
   virtual int getSolveCount() const
   {
      return m_vanished ? 0 : m_slu.getSolveCount();
   }
   ///
   virtual int iteration() const
   {      
      return m_vanished ? 0 : m_solver.basis().iteration();
   }
   ///
   virtual bool terminate() 
   {
      return m_solver.terminate();
   }
   /// returns the current status
   virtual SPxSolver::Status status() const
   {
      if (m_vanished)
         return SPxSolver::OPTIMAL;

      return m_solver.status();
   }
   //@}

   //---------------------------------------
   //**@name I/O */
   //@{

   /** Load basis from \p filename in MPS format. If \p rowNames and \p
    *  colNames are \c NULL, default names are used for the constraints and
    *  variables.
    */
   virtual bool readBasisFile(const char* filename, 
      const NameSet* rowNames, const NameSet* colNames);

   /** Write basis to \p filename in MPS format. If \p rowNames and \p
    *  colNames are \c NULL, default names are used for the constraints and
    *  variables.
    */
   virtual bool writeBasisFile(const char* filename, 
      const NameSet* rowNames, const NameSet* colNames ) const;

   /** Write LP, basis and parameter settings of the current SPxSolver object
    *  (i.e. after simplifying and scaling).
    *  LP is written in MPS format to "\p filename".mps, basis is written in
    *  "\p filename".bas, and parameters are written to "\p filename".set.
    *  If \p rowNames and \p colNames are \c NULL, default names are used for
    *  the constraints and variables.
    */
   virtual bool writeState(const char* filename, 
      const NameSet* rowNames = NULL, const NameSet* colNames = NULL) const;

   /// returns statistical information in form of a string.
   std::string statistics() const
   {
      return m_solver.statistics();
   }
   //@}

private:

   //------------------------------------
   //**@name Private helpers */
   //@{
   /// undoes preprocessing such that the unsimplified solution values and basis is available
   void unsimplify() const;
   //@}
};
} // namespace soplex
#endif // _SOPLEX_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------
