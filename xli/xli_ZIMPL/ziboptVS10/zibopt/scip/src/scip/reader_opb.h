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
#pragma ident "@(#) $Id: reader_opb.h,v 1.6 2010/04/26 14:39:08 bzfwinkm Exp $"

/**@file   reader_opb.h
 * @brief  pseudo-Boolean file reader (opb format)
 * @author Stefan Heinz
 * @author Michael Winkler
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_READER_OPB_H__
#define __SCIP_READER_OPB_H__

#include "scip/scip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** includes the opb file reader into SCIP */
extern
SCIP_RETCODE SCIPincludeReaderOpb(
   SCIP*                 scip                /**< SCIP data structure */
   );

/* reads problem from file */
extern
SCIP_RETCODE SCIPreadOpb(
   SCIP*              scip,               /**< SCIP data structure */
   SCIP_READER*       reader,             /**< the file reader itself */
   const char*        filename,           /**< full path and name of file to read, or NULL if stdin should be used */
   SCIP_RESULT*       result              /**< pointer to store the result of the file reading call */
   );

/* writes problem to file */
extern
SCIP_RETCODE SCIPwriteOpb(
   SCIP*              scip,               /**< SCIP data structure */
   FILE*              file,               /**< output file, or NULL if standard output should be used */
   const char*        name,               /**< problem name */
   SCIP_Bool          transformed,        /**< TRUE iff problem is the transformed problem */
   SCIP_OBJSENSE      objsense,           /**< objective sense */
   SCIP_Real          objscale,           /**< scalar applied to objective function; external objective value is
					     extobj = objsense * objscale * (intobj + objoffset) */
   SCIP_Real          objoffset,          /**< objective offset from bound shifting and fixing */
   SCIP_VAR**         vars,               /**< array with active variables ordered binary, integer, implicit, continuous */
   int                nvars,              /**< number of mutable variables in the problem */
   int                nbinvars,           /**< number of binary variables */
   int                nintvars,           /**< number of general integer variables */
   int                nimplvars,          /**< number of implicit integer variables */
   int                ncontvars,          /**< number of continuous variables */
   int                nfixedvars,         /**< number of fixed and aggregated variables in the problem */
   SCIP_CONS**        conss,              /**< array with constraints of the problem */
   int                nconss,             /**< number of constraints in the problem */
   SCIP_Bool          genericnames,       /**< should generic variable and constraint names be used */
   SCIP_RESULT*       result              /**< pointer to store the result of the file writing call */
   );

#ifdef __cplusplus
}
#endif

#endif
