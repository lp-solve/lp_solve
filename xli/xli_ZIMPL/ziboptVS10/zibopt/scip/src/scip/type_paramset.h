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
#pragma ident "@(#) $Id: type_paramset.h,v 1.20 2010/09/10 15:00:48 bzfwolte Exp $"

/**@file   type_paramset.h
 * @ingroup TYPEDEFINITIONS
 * @brief  type definitions for handling parameter settings
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_TYPE_PARAMSET_H__
#define __SCIP_TYPE_PARAMSET_H__

#include "scip/def.h"
#include "scip/type_retcode.h"
#include "scip/type_scip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** possible parameter types */
enum SCIP_ParamType
{
   SCIP_PARAMTYPE_BOOL    = 0,               /**< bool values: TRUE or FALSE */
   SCIP_PARAMTYPE_INT     = 1,               /**< integer values */
   SCIP_PARAMTYPE_LONGINT = 2,               /**< long integer values */
   SCIP_PARAMTYPE_REAL    = 3,               /**< real values */
   SCIP_PARAMTYPE_CHAR    = 4,               /**< characters */
   SCIP_PARAMTYPE_STRING  = 5                /**< strings: arrays of characters */
};
typedef enum SCIP_ParamType SCIP_PARAMTYPE;

/** possible parameter settings */
enum SCIP_ParamSetting
{
   SCIP_PARAMSETTING_DEFAULT     = 0,        /**< use default values */

   SCIP_PARAMSETTING_AGGRESSIVE  = 1,        /**< set to aggressive settings */
   SCIP_PARAMSETTING_FAST        = 2,        /**< set to fast settings */
   SCIP_PARAMSETTING_OFF         = 3,        /**< turn off */

   SCIP_PARAMSETTING_COUNTER     = 4,        /**< get a feasible and "fast" counting process */
   SCIP_PARAMSETTING_CPSOLVER    = 5,        /**< get CP like search (e.g. no LP relaxation) */
   SCIP_PARAMSETTING_EASYCIP     = 6,        /**< solve easy problems fast */
   SCIP_PARAMSETTING_FEASIBILITY = 7,        /**< detect feasibility fast */
   SCIP_PARAMSETTING_HARDLP      = 8,        /**< be capable to handle hard LPs */
   SCIP_PARAMSETTING_OPTIMALITY  = 9         /**< prove optimality fast */
};
typedef enum SCIP_ParamSetting SCIP_PARAMSETTING;

typedef struct SCIP_Param SCIP_PARAM;             /**< single parameter */
typedef struct SCIP_ParamData SCIP_PARAMDATA;     /**< locally defined parameter specific data */
typedef struct SCIP_ParamSet SCIP_PARAMSET;       /**< set of parameters */


/** information method for changes in the parameter
 *
 *  Method is called if the parameter was changed through a SCIPparamsetSetXxx() call
 *  (which is called by SCIPsetXxxParam()).
 *  It will not be called, if the parameter was changed directly by changing the value
 *  in the memory location.
 *
 *  input:
 *    scip            : SCIP main data structure
 *    param           : the changed parameter (already set to its new value)
 */
#define SCIP_DECL_PARAMCHGD(x) SCIP_RETCODE x (SCIP* scip, SCIP_PARAM* param)

#ifdef __cplusplus
}
#endif

#endif
