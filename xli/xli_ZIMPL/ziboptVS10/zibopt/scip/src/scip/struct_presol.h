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
#pragma ident "@(#) $Id: struct_presol.h,v 1.21 2010/03/12 14:54:30 bzfwinkm Exp $"

/**@file   struct_presol.h
 * @brief  datastructures for presolvers
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_STRUCT_PRESOL_H__
#define __SCIP_STRUCT_PRESOL_H__


#include "scip/def.h"
#include "scip/type_clock.h"
#include "scip/type_presol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** presolver */
struct SCIP_Presol
{
   char*                 name;               /**< name of presolver */
   char*                 desc;               /**< description of presolver */
   SCIP_DECL_PRESOLCOPY  ((*presolcopy));    /**< copy method of presolver or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_PRESOLFREE  ((*presolfree));    /**< destructor of presolver to free user data (called when SCIP is exiting) */
   SCIP_DECL_PRESOLINIT  ((*presolinit));    /**< initialization method of presolver (called after problem was transformed) */
   SCIP_DECL_PRESOLEXIT  ((*presolexit));    /**< deinitialization method of presolver (called before transformed problem is freed) */
   SCIP_DECL_PRESOLINITPRE((*presolinitpre));/**< presolving initialization method of presolver (called when presolving is about to begin) */
   SCIP_DECL_PRESOLEXITPRE((*presolexitpre));/**< presolving deinitialization method of presolver (called after presolving has been finished) */
   SCIP_DECL_PRESOLEXEC  ((*presolexec));    /**< execution method of presolver */
   SCIP_PRESOLDATA*      presoldata;         /**< presolver data */
   SCIP_CLOCK*           presolclock;        /**< presolving time */
   int                   priority;           /**< priority of the presolver */
   int                   maxrounds;          /**< maximal number of presolving rounds the presolver participates in (-1: no limit) */
   int                   lastnfixedvars;     /**< number of variables fixed before the last call to the presolver */
   int                   lastnaggrvars;      /**< number of variables aggregated before the last call to the presolver */
   int                   lastnchgvartypes;   /**< number of variable type changes before the last call to the presolver */
   int                   lastnchgbds;        /**< number of variable bounds tightend before the last call to the presolver */
   int                   lastnaddholes;      /**< number of domain holes added before the last call to the presolver */
   int                   lastndelconss;      /**< number of deleted constraints before the last call to the presolver */
   int                   lastnupgdconss;     /**< number of upgraded constraints before the last call to the presolver */
   int                   lastnchgcoefs;      /**< number of changed coefficients before the last call to the presolver */
   int                   lastnchgsides;      /**< number of changed left or right hand sides before the last call */
   int                   nfixedvars;         /**< total number of variables fixed by this presolver */
   int                   naggrvars;          /**< total number of variables aggregated by this presolver */
   int                   nchgvartypes;       /**< total number of variable type changes by this presolver */
   int                   nchgbds;            /**< total number of variable bounds tightend by this presolver */
   int                   naddholes;          /**< total number of domain holes added by this presolver */
   int                   ndelconss;          /**< total number of deleted constraints by this presolver */
   int                   nupgdconss;         /**< total number of upgraded constraints by this presolver */
   int                   nchgcoefs;          /**< total number of changed coefficients by this presolver */
   int                   nchgsides;          /**< total number of changed left or right hand sides by this presolver */
   SCIP_Bool             delay;              /**< should presolver be delayed, if other presolvers found reductions? */
   SCIP_Bool             wasdelayed;         /**< was the presolver delayed at the last call? */
   SCIP_Bool             initialized;        /**< is presolver initialized? */
};

#ifdef __cplusplus
}
#endif

#endif
