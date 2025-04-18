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
#pragma ident "@(#) $Id: type_branch.h,v 1.29 2010/09/29 20:24:56 bzfgamra Exp $"

/**@file   type_branch.h
 * @ingroup TYPEDEFINITIONS
 * @brief  type definitions for branching rules
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_TYPE_BRANCH_H__
#define __SCIP_TYPE_BRANCH_H__

#include "scip/def.h"
#include "scip/type_result.h"
#include "scip/type_scip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SCIP_BranchCand SCIP_BRANCHCAND;   /**< branching candidate storage */
typedef struct SCIP_Branchrule SCIP_BRANCHRULE;   /**< branching method data structure */
typedef struct SCIP_BranchruleData SCIP_BRANCHRULEDATA; /**< branching method specific data */


/** copy method for branchrule plugins (called when SCIP copies plugins)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHCOPY(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** destructor of branching method to free user data (called when SCIP is exiting)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHFREE(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** initialization method of branching rule (called after problem was transformed)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHINIT(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** deinitialization method of branching rule (called before transformed problem is freed)
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHEXIT(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** solving process initialization method of branching rule (called when branch and bound process is about to begin)
 *
 *  This method is called when the presolving was finished and the branch and bound process is about to begin.
 *  The branching rule may use this call to initialize its branch and bound specific data.
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHINITSOL(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** solving process deinitialization method of branching rule (called before branch and bound process data is freed)
 *
 *  This method is called before the branch and bound process is freed.
 *  The branching rule should use this call to clean up its branch and bound data.
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 */
#define SCIP_DECL_BRANCHEXITSOL(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule)

/** branching execution method for fractional LP solutions
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 *  - allowaddcons    : is the branching rule allowed to add constraints to the current node in order to cut off the
 *                      current solution instead of creating a branching?
 *  - result          : pointer to store the result of the branching call
 *
 *  possible return values for *result (if more than one applies, the first in the list should be used):
 *  - SCIP_CUTOFF     : the current node was detected to be infeasible
 *  - SCIP_CONSADDED  : an additional constraint (e.g. a conflict constraint) was generated; this result code must not be
 *                      returned, if allowaddcons is FALSE
 *  - SCIP_REDUCEDDOM : a domain was reduced that rendered the current LP solution infeasible
 *  - SCIP_SEPARATED  : a cutting plane was generated
 *  - SCIP_BRANCHED   : branching was applied
 *  - SCIP_DIDNOTRUN  : the branching rule was skipped
 */
#define SCIP_DECL_BRANCHEXECLP(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule, SCIP_Bool allowaddcons, SCIP_RESULT* result)


/** branching execution method for external candidates
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 *  - allowaddcons    : is the branching rule allowed to add constraints to the current node in order to cut off the
 *                      current solution instead of creating a branching?
 *  - result          : pointer to store the result of the branching call
 *
 *  possible return values for *result (if more than one applies, the first in the list should be used):
 *  - SCIP_CUTOFF     : the current node was detected to be infeasible
 *  - SCIP_CONSADDED  : an additional constraint (e.g. a conflict constraint) was generated; this result code must not be
 *                      returned, if allowaddcons is FALSE
 *  - SCIP_REDUCEDDOM : a domain was reduced that rendered the current pseudo solution infeasible
 *  - SCIP_BRANCHED   : branching was applied
 *  - SCIP_DIDNOTRUN  : the branching rule was skipped
 */
#define SCIP_DECL_BRANCHEXECEXT(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule, SCIP_Bool allowaddcons, SCIP_RESULT* result)


/** branching execution method for not completely fixed pseudo solutions
 *
 *  input:
 *  - scip            : SCIP main data structure
 *  - branchrule      : the branching rule itself
 *  - allowaddcons    : is the branching rule allowed to add constraints to the current node in order to cut off the
 *                      current solution instead of creating a branching?
 *  - result          : pointer to store the result of the branching call
 *
 *  possible return values for *result (if more than one applies, the first in the list should be used):
 *  - SCIP_CUTOFF     : the current node was detected to be infeasible
 *  - SCIP_CONSADDED  : an additional constraint (e.g. a conflict constraint) was generated; this result code must not be
 *                      returned, if allowaddcons is FALSE
 *  - SCIP_REDUCEDDOM : a domain was reduced that rendered the current pseudo solution infeasible
 *  - SCIP_BRANCHED   : branching was applied
 *  - SCIP_DIDNOTRUN  : the branching rule was skipped
 */
#define SCIP_DECL_BRANCHEXECPS(x) SCIP_RETCODE x (SCIP* scip, SCIP_BRANCHRULE* branchrule, SCIP_Bool allowaddcons, SCIP_RESULT* result)

#ifdef __cplusplus
}
#endif

#endif
