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
#pragma ident "@(#) $Id: sepa.h,v 1.54 2010/09/27 17:20:24 bzfheinz Exp $"

/**@file   sepa.h
 * @brief  internal methods for separators
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_SEPA_H__
#define __SCIP_SEPA_H__


#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_retcode.h"
#include "scip/type_result.h"
#include "scip/type_set.h"
#include "scip/type_stat.h"
#include "scip/type_sepastore.h"
#include "scip/type_sepa.h"
#include "scip/pub_sepa.h"

#ifdef __cplusplus
extern "C" {
#endif

/** copies the given separator to a new scip */
extern
SCIP_RETCODE SCIPsepaCopyInclude(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set                 /**< SCIP_SET of SCIP to copy to */
   );

/** creates a separator */
extern
SCIP_RETCODE SCIPsepaCreate(
   SCIP_SEPA**           sepa,               /**< pointer to separator data structure */
   SCIP_SET*             set,                /**< global SCIP settings */
   BMS_BLKMEM*           blkmem,             /**< block memory for parameter settings */
   const char*           name,               /**< name of separator */
   const char*           desc,               /**< description of separator */
   int                   priority,           /**< priority of separator (>= 0: before, < 0: after constraint handlers) */
   int                   freq,               /**< frequency for calling separator */
   SCIP_Real             maxbounddist,       /**< maximal relative distance from current node's dual bound to primal bound compared
                                              *   to best node's dual bound for applying separation */
   SCIP_Bool             usessubscip,        /**< does the separator use a secondary SCIP instance? */
   SCIP_Bool             delay,              /**< should separator be delayed, if other separators found cuts? */
   SCIP_DECL_SEPACOPY    ((*sepacopy)),      /**< copy method of separator or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_SEPAFREE    ((*sepafree)),      /**< destructor of separator */
   SCIP_DECL_SEPAINIT    ((*sepainit)),      /**< initialize separator */
   SCIP_DECL_SEPAEXIT    ((*sepaexit)),      /**< deinitialize separator */
   SCIP_DECL_SEPAINITSOL ((*sepainitsol)),   /**< solving process initialization method of separator */
   SCIP_DECL_SEPAEXITSOL ((*sepaexitsol)),   /**< solving process deinitialization method of separator */
   SCIP_DECL_SEPAEXECLP  ((*sepaexeclp)),    /**< LP solution separation method of separator */
   SCIP_DECL_SEPAEXECSOL ((*sepaexecsol)),   /**< arbitrary primal solution separation method of separator */
   SCIP_SEPADATA*        sepadata            /**< separator data */
   );

/** calls destructor and frees memory of separator */
extern
SCIP_RETCODE SCIPsepaFree(
   SCIP_SEPA**           sepa,               /**< pointer to separator data structure */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** initializes separator */
extern
SCIP_RETCODE SCIPsepaInit(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** calls exit method of separator */
extern
SCIP_RETCODE SCIPsepaExit(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs separator that the branch and bound process is being started */
extern
SCIP_RETCODE SCIPsepaInitsol(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs separator that the branch and bound process data is being freed */
extern
SCIP_RETCODE SCIPsepaExitsol(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** calls LP separation method of separator */
extern
SCIP_RETCODE SCIPsepaExecLP(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_STAT*            stat,               /**< dynamic problem statistics */
   SCIP_SEPASTORE*       sepastore,          /**< separation storage */
   int                   depth,              /**< depth of current node */
   SCIP_Real             bounddist,          /**< current relative distance of local dual bound to global dual bound */
   SCIP_Bool             execdelayed,        /**< execute separator even if it is marked to be delayed */
   SCIP_RESULT*          result              /**< pointer to store the result of the callback method */
   );

/** calls primal solution separation method of separator */
extern
SCIP_RETCODE SCIPsepaExecSol(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_STAT*            stat,               /**< dynamic problem statistics */
   SCIP_SEPASTORE*       sepastore,          /**< separation storage */
   SCIP_SOL*             sol,                /**< primal solution that should be separated */
   int                   depth,              /**< depth of current node */
   SCIP_Bool             execdelayed,        /**< execute separator even if it is marked to be delayed */
   SCIP_RESULT*          result              /**< pointer to store the result of the callback method */
   );

/** sets priority of separator */
extern
void SCIPsepaSetPriority(
   SCIP_SEPA*            sepa,               /**< separator */
   SCIP_SET*             set,                /**< global SCIP settings */
   int                   priority            /**< new priority of the separator */
   );

#ifdef __cplusplus
}
#endif

#endif
