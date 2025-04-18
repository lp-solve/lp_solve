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
#pragma ident "@(#) $Id: cons_logicor.h,v 1.33 2010/01/04 20:35:38 bzfheinz Exp $"

/**@file   cons_logicor.h
 * @brief  constraint handler for logicor constraints
 *         (equivalent to set covering, but algorithms are suited for depth first search)
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_CONS_LOGICOR_H__
#define __SCIP_CONS_LOGICOR_H__


#include "scip/scip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates the handler for logic or constraints and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeConshdlrLogicor(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** creates and captures a logic or constraint */
extern
SCIP_RETCODE SCIPcreateConsLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   nvars,              /**< number of variables in the constraint */
   SCIP_VAR**            vars,               /**< array with variables of constraint entries */
   SCIP_Bool             initial,            /**< should the LP relaxation of constraint be in the initial LP?
                                              *   Usually set to TRUE. Set to FALSE for 'lazy constraints'. */
   SCIP_Bool             separate,           /**< should the constraint be separated during LP processing?
                                              *   Usually set to TRUE. */
   SCIP_Bool             enforce,            /**< should the constraint be enforced during node processing?
                                              *   TRUE for model constraints, FALSE for additional, redundant constraints. */
   SCIP_Bool             check,              /**< should the constraint be checked for feasibility?
                                              *   TRUE for model constraints, FALSE for additional, redundant constraints. */
   SCIP_Bool             propagate,          /**< should the constraint be propagated during node processing?
                                              *   Usually set to TRUE. */
   SCIP_Bool             local,              /**< is constraint only valid locally?
                                              *   Usually set to FALSE. Has to be set to TRUE, e.g., for branching constraints. */
   SCIP_Bool             modifiable,         /**< is constraint modifiable (subject to column generation)?
                                              *   Usually set to FALSE. In column generation applications, set to TRUE if pricing
                                              *   adds coefficients to this constraint. */
   SCIP_Bool             dynamic,            /**< is constraint subject to aging?
                                              *   Usually set to FALSE. Set to TRUE for own cuts which 
                                              *   are seperated as constraints. */
   SCIP_Bool             removable,          /**< should the relaxation be removed from the LP due to aging or cleanup?
                                              *   Usually set to FALSE. Set to TRUE for 'lazy constraints' and 'user cuts'. */
   SCIP_Bool             stickingatnode      /**< should the constraint always be kept at the node where it was added, even
                                              *   if it may be moved to a more global node?
                                              *   Usually set to FALSE. Set to TRUE to for constraints that represent node data. */
   );

/** gets number of variables in logic or constraint */
extern
int SCIPgetNVarsLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   );

/** gets array of variables in logic or constraint */
extern
SCIP_VAR** SCIPgetVarsLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   );

/** gets the dual solution of the logic or constraint in the current LP */
extern
SCIP_Real SCIPgetDualsolLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   );

/** gets the dual farkas value of the logic or constraint in the current infeasible LP */
extern
SCIP_Real SCIPgetDualfarkasLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   );

/** returns the linear relaxation of the given logic or constraint; may return NULL if no LP row was yet created;
 *  the user must not modify the row!
 */
extern
SCIP_ROW* SCIPgetRowLogicor(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS*            cons                /**< constraint data */
   );

#ifdef __cplusplus
}
#endif

#endif
