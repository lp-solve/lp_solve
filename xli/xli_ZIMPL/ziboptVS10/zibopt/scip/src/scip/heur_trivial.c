/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: heur_trivial.c,v 1.21 2010/09/27 17:20:22 bzfheinz Exp $"


/**@file   heur_trivial.c
 * @ingroup PRIMALHEURISTICS
 * @brief  trivial primal heuristic
 * @author Timo Berthold
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

#include "scip/heur_trivial.h"


#define HEUR_NAME             "trivial"
#define HEUR_DESC             "start heuristic which tries some trivial solutions"
#define HEUR_DISPCHAR         't'
#define HEUR_PRIORITY         10000
#define HEUR_FREQ             0
#define HEUR_FREQOFS          0
#define HEUR_MAXDEPTH         -1
#define HEUR_TIMING            SCIP_HEURTIMING_BEFOREPRESOL | SCIP_HEURTIMING_BEFORENODE
#define HEUR_USESSUBSCIP      FALSE  /**< does the heuristic use a secondary SCIP instance? */

/*
 * Local methods
 */

/** copy method for primal heuristic plugins (called when SCIP copies plugins) */
static
SCIP_DECL_HEURCOPY(heurCopyTrivial)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(heur != NULL);
   assert(strcmp(SCIPheurGetName(heur), HEUR_NAME) == 0);

   /* call inclusion method of primal heuristic */
   SCIP_CALL( SCIPincludeHeurTrivial(scip) );

   return SCIP_OKAY;
}

#define heurFreeTrivial NULL
#define heurInitTrivial NULL
#define heurExitTrivial NULL
#define heurInitsolTrivial NULL
#define heurExitsolTrivial NULL


/** execution method of primal heuristic */
static
SCIP_DECL_HEUREXEC(heurExecTrivial)
{  /*lint --e{715}*/
   SCIP_VAR** vars;
   SCIP_SOL* lbsol;                     /* solution where all variables are set to their lower bounds */
   SCIP_SOL* ubsol;                     /* solution where all variables are set to their upper bounds */
   SCIP_SOL* zerosol;                   /* solution where all variables are set to zero */
   SCIP_SOL* locksol;                   /* solution where all variables are set to the bound with the fewer locks */

   SCIP_Real infinity;

   int nvars;
   int nbinvars;
   int i;

   SCIP_Bool success;
   SCIP_Bool zerovalid;

   *result = SCIP_DIDNOTRUN;

   if( SCIPgetNRuns(scip) > 1 )
      return SCIP_OKAY;

   if( SCIPgetNBinVars(scip) + SCIPgetNIntVars(scip) == 0 )
      return SCIP_OKAY;

   *result = SCIP_DIDNOTFIND;
   success = FALSE;

   /* initialize data structure */
   SCIP_CALL( SCIPcreateSol(scip, &lbsol, heur) );
   SCIP_CALL( SCIPcreateSol(scip, &ubsol, heur) );
   SCIP_CALL( SCIPcreateSol(scip, &zerosol, heur) );
   SCIP_CALL( SCIPcreateSol(scip, &locksol, heur) );
   
   infinity = SCIPinfinity(scip);
   infinity = MIN(100000.0, infinity);
   
   SCIP_CALL( SCIPgetVarsData(scip, &vars, &nvars, &nbinvars, NULL, NULL, NULL) );

   /* if the problem is binary, we do not have to check the zero solution, since it is equal to the lower bound
    * solution */
   zerovalid = (nvars != nbinvars);   
   assert(vars != NULL || nvars == 0);

   for( i = 0; i < nvars; i++ )
   {
      SCIP_Real lb;
      SCIP_Real ub;
      
      assert(vars != NULL); /* this assert is needed for flexelint */
      
      lb = SCIPvarGetLbLocal(vars[i]);
      ub = SCIPvarGetUbLocal(vars[i]);

      /* set infinite bounds to sufficient large value */
      if( SCIPisInfinity(scip, -lb) )
         lb = MIN(-infinity, ub);
      if( SCIPisInfinity(scip, ub) )
      {
         SCIP_Real tmp;

         tmp = SCIPvarGetLbLocal(vars[i]);
         ub = MAX(tmp, infinity);
      }

      SCIP_CALL( SCIPsetSolVal(scip, lbsol, vars[i], lb) );
      SCIP_CALL( SCIPsetSolVal(scip, ubsol, vars[i], ub) );

      /* try the zero vector, if it is in the bounds region */
      if( zerovalid )
      {
         if( SCIPisLE(scip, lb, 0.0) && SCIPisLE(scip, 0.0, ub) )
         {
            SCIP_CALL( SCIPsetSolVal(scip, zerosol, vars[i], 0.0) );
         }
         else
            zerovalid = FALSE;         
      }

      /* set variables to the bound with fewer locks, if tie choose an average value */
      if( SCIPvarGetNLocksDown(vars[i]) >  SCIPvarGetNLocksUp(vars[i]) )
      {
         SCIP_CALL( SCIPsetSolVal(scip, locksol, vars[i], ub) );
      }
      else if( SCIPvarGetNLocksDown(vars[i]) <  SCIPvarGetNLocksUp(vars[i]) )
      {
         SCIP_CALL( SCIPsetSolVal(scip, locksol, vars[i], lb) );
      }      
      else
      {
         SCIP_Real solval;
         solval = (lb+ub)/2.0;
         
         /* if a tie occurs, roughly every third integer variable will be rounded up */
         if( SCIPvarGetType(vars[i]) != SCIP_VARTYPE_CONTINUOUS )
            solval = i % 3 == 0 ? SCIPceil(scip,solval) : SCIPfloor(scip,solval);

         assert(SCIPisFeasLE(scip,SCIPvarGetLbLocal(vars[i]),solval) && SCIPisFeasLE(scip,solval,SCIPvarGetUbLocal(vars[i])));
         
         SCIP_CALL( SCIPsetSolVal(scip, locksol, vars[i], solval) );
      }
   }
   
   /* try lower bound solution */
   SCIP_CALL( SCIPtrySol(scip, lbsol, FALSE, FALSE, TRUE, TRUE, &success) );

   if( success )
   {
      SCIPdebugMessage("found feasible lower bound solution:\n");
      SCIPdebug( SCIP_CALL( SCIPprintSol(scip, lbsol, NULL, FALSE) ) );

      *result = SCIP_FOUNDSOL;
   }

   /* try upper bound solution */   
   SCIP_CALL( SCIPtrySol(scip, ubsol, FALSE, FALSE, TRUE, TRUE, &success) );

   if( success )
   {
      SCIPdebugMessage("found feasible upper bound solution:\n");
      SCIPdebug( SCIP_CALL( SCIPprintSol(scip, ubsol, NULL, FALSE) ) );

      *result = SCIP_FOUNDSOL;
   }

   /* try zero solution */
   if( zerovalid )
   {
      SCIP_CALL( SCIPtrySol(scip, zerosol, FALSE, FALSE, TRUE, TRUE, &success) );
      
      if( success )
      {
         SCIPdebugMessage("found feasible zero solution:\n");
         SCIPdebug( SCIP_CALL( SCIPprintSol(scip, zerosol, NULL, FALSE) ) );

         *result = SCIP_FOUNDSOL;
      }
   }

   /* try lock solution */
   SCIP_CALL( SCIPtrySol(scip, locksol, FALSE, FALSE, TRUE, TRUE, &success) );

   if( success )
   {
      SCIPdebugMessage("found feasible lock solution:\n");
      SCIPdebug( SCIP_CALL( SCIPprintSol(scip, locksol, NULL, FALSE) ) );

      *result = SCIP_FOUNDSOL;
   }

   /* free solutions */
   SCIP_CALL( SCIPfreeSol(scip, &lbsol) );
   SCIP_CALL( SCIPfreeSol(scip, &ubsol) );
   SCIP_CALL( SCIPfreeSol(scip, &zerosol) );
   SCIP_CALL( SCIPfreeSol(scip, &locksol) );

   return SCIP_OKAY;
}


/*
 * primal heuristic specific interface methods
 */

/** creates the trivial primal heuristic and includes it in SCIP */
SCIP_RETCODE SCIPincludeHeurTrivial(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   /* include primal heuristic */
   SCIP_CALL( SCIPincludeHeur(scip, HEUR_NAME, HEUR_DESC, HEUR_DISPCHAR, HEUR_PRIORITY, HEUR_FREQ, HEUR_FREQOFS,
         HEUR_MAXDEPTH, HEUR_TIMING, HEUR_USESSUBSCIP,
         heurCopyTrivial,
         heurFreeTrivial, heurInitTrivial, heurExitTrivial, 
         heurInitsolTrivial, heurExitsolTrivial, heurExecTrivial,
         NULL) );

   return SCIP_OKAY;
}
