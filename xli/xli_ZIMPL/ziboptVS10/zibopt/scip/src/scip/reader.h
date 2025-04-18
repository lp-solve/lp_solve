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
#pragma ident "@(#) $Id: reader.h,v 1.37 2010/09/27 17:20:23 bzfheinz Exp $"

/**@file   reader.h
 * @brief  internal methods for input file readers
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_READER_H__
#define __SCIP_READER_H__


#include "scip/def.h"
#include "scip/type_prob.h"
#include "scip/type_retcode.h"
#include "scip/type_result.h"
#include "scip/type_set.h"
#include "scip/type_reader.h"
#include "scip/pub_reader.h"

#ifdef __cplusplus
extern "C" {
#endif


/** copies the given reader to a new scip */
extern
SCIP_RETCODE SCIPreaderCopyInclude(
   SCIP_READER*          reader,             /**< reader */
   SCIP_SET*             set                 /**< SCIP_SET of SCIP to copy to */
   );

/** creates a reader */
extern
SCIP_RETCODE SCIPreaderCreate(
   SCIP_READER**         reader,             /**< pointer to store reader */
   const char*           name,               /**< name of reader */
   const char*           desc,               /**< description of reader */
   const char*           extension,          /**< file extension that reader processes */
   SCIP_DECL_READERCOPY  ((*readercopy)),    /**< copy method of reader or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_READERFREE  ((*readerfree)),    /**< destructor of reader */
   SCIP_DECL_READERREAD  ((*readerread)),    /**< read method */
   SCIP_DECL_READERWRITE ((*readerwrite)),   /**< write method */
   SCIP_READERDATA*      readerdata          /**< reader data */
   );

/** frees memory of reader */
extern
SCIP_RETCODE SCIPreaderFree(
   SCIP_READER**         reader,             /**< pointer to reader data structure */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** reads problem data from file with given reader or returns SCIP_DIDNOTRUN */
extern
SCIP_RETCODE SCIPreaderRead(
   SCIP_READER*          reader,             /**< reader */
   SCIP_SET*             set,                /**< global SCIP settings */
   const char*           filename,           /**< name of the input file */
   const char*           extension,          /**< extension of the input file name */
   SCIP_RESULT*          result              /**< pointer to store the result of the callback method */
   );

/** writes problem data to file with given reader or returns SCIP_DIDNOTRUN */
extern
SCIP_RETCODE SCIPreaderWrite(
   SCIP_READER*          reader,             /**< reader */
   SCIP_PROB*            prob,               /**< problem data */
   SCIP_SET*             set,                /**< global SCIP settings */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   const char*           format,             /**< file format (or NULL) */
   SCIP_Bool             genericnames,       /**< using generic variable and constraint names? */
   SCIP_RESULT*          result              /**< pointer to store the result of the callback method */
   );

#ifdef __cplusplus
}
#endif

#endif
