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
#pragma ident "@(#) $Id: pricer.h,v 1.30 2010/09/08 19:14:55 bzfhende Exp $"

/**@file   pricer.h
 * @brief  internal methods for variable pricers
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_PRICER_H__
#define __SCIP_PRICER_H__


#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_retcode.h"
#include "scip/type_result.h"
#include "scip/type_set.h"
#include "scip/type_lp.h"
#include "scip/type_prob.h"
#include "scip/type_pricestore.h"
#include "scip/type_pricer.h"
#include "scip/pub_pricer.h"

#ifdef __cplusplus
extern "C" {
#endif


/** copies the given pricer to a new scip */
extern
SCIP_RETCODE SCIPpricerCopyInclude(
   SCIP_PRICER*          pricer,             /**< pricer */
   SCIP_SET*             set,                /**< SCIP_SET of SCIP to copy to */
   SCIP_Bool*            valid               /**< was the copying process valid? */
   );

/** creates a variable pricer */
extern
SCIP_RETCODE SCIPpricerCreate(
   SCIP_PRICER**         pricer,             /**< pointer to variable pricer data structure */
   SCIP_SET*             set,                /**< global SCIP settings */
   BMS_BLKMEM*           blkmem,             /**< block memory for parameter settings */
   const char*           name,               /**< name of variable pricer */
   const char*           desc,               /**< description of variable pricer */
   int                   priority,           /**< priority of the variable pricer */
   SCIP_Bool             delay,              /**< should the pricer be delayed until no other pricers or already existing
                                              *   problem variables with negative reduced costs are found */
   SCIP_DECL_PRICERCOPY  ((*pricercopy)),    /**< copy method of pricer or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_PRICERFREE  ((*pricerfree)),    /**< destructor of variable pricer */
   SCIP_DECL_PRICERINIT  ((*pricerinit)),    /**< initialize variable pricer */
   SCIP_DECL_PRICEREXIT  ((*pricerexit)),    /**< deinitialize variable pricer */
   SCIP_DECL_PRICERINITSOL((*pricerinitsol)),/**< solving process initialization method of variable pricer */
   SCIP_DECL_PRICEREXITSOL((*pricerexitsol)),/**< solving process deinitialization method of variable pricer */
   SCIP_DECL_PRICERREDCOST((*pricerredcost)),/**< reduced cost pricing method of variable pricer for feasible LPs */
   SCIP_DECL_PRICERFARKAS((*pricerfarkas)),  /**< farkas pricing method of variable pricer for infeasible LPs */
   SCIP_PRICERDATA*      pricerdata          /**< variable pricer data */
   );

/** calls destructor and frees memory of variable pricer */
extern
SCIP_RETCODE SCIPpricerFree(
   SCIP_PRICER**         pricer,             /**< pointer to variable pricer data structure */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** initializes variable pricer */
extern
SCIP_RETCODE SCIPpricerInit(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** calls exit method of variable pricer */
extern
SCIP_RETCODE SCIPpricerExit(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs variable pricer that the branch and bound process is being started */
extern
SCIP_RETCODE SCIPpricerInitsol(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs variable pricer that the branch and bound process data is being freed */
extern
SCIP_RETCODE SCIPpricerExitsol(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** activates pricer such that it is called in LP solving loop */
extern
SCIP_RETCODE SCIPpricerActivate(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** deactivates pricer such that it is no longer called in LP solving loop */
extern
SCIP_RETCODE SCIPpricerDeactivate(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** calls reduced cost pricing method of variable pricer */
extern
SCIP_RETCODE SCIPpricerRedcost(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_PROB*            prob,               /**< transformed problem */
   SCIP_Real*            lowerbound,         /**< local lower bound computed by the pricer */
   SCIP_RESULT*          result              /**< result of the pricing process */    
   );

/** calls farkas pricing method of variable pricer */
extern
SCIP_RETCODE SCIPpricerFarkas(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_PROB*            prob                /**< transformed problem */
   );

/** depending on the LP's solution status, calls reduced cost or farkas pricing method of variable pricer */
extern
SCIP_RETCODE SCIPpricerExec(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_PROB*            prob,               /**< transformed problem */
   SCIP_LP*              lp,                 /**< LP data */
   SCIP_PRICESTORE*      pricestore,         /**< pricing storage */
   SCIP_Real*            lowerbound,         /**< local lower bound computed by the pricer */
   SCIP_RESULT*          result              /**< result of the pricing process */
   );

/** sets priority of variable pricer */
extern
void SCIPpricerSetPriority(
   SCIP_PRICER*          pricer,             /**< variable pricer */
   SCIP_SET*             set,                /**< global SCIP settings */
   int                   priority            /**< new priority of the variable pricer */
   );

#ifdef __cplusplus
}
#endif

#endif
