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
#pragma ident "@(#) $Id: disp.h,v 1.40 2010/09/27 17:20:21 bzfheinz Exp $"

/**@file   disp.h
 * @brief  internal methods for displaying runtime statistics
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_DISP_H__
#define __SCIP_DISP_H__


#include <stdio.h>

#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_retcode.h"
#include "scip/type_set.h"
#include "scip/type_stat.h"
#include "scip/type_disp.h"
#include "scip/pub_disp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** parameter change information method to autoselect display columns again */
extern
SCIP_DECL_PARAMCHGD(SCIPparamChgdDispActive);

/** copies the given display to a new scip */
extern
SCIP_RETCODE SCIPdispCopyInclude(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set                 /**< SCIP_SET of SCIP to copy to */
   );

/** creates a display column */
extern
SCIP_RETCODE SCIPdispCreate(
   SCIP_DISP**           disp,               /**< pointer to store display column */
   SCIP_SET*             set,                /**< global SCIP settings */
   BMS_BLKMEM*           blkmem,             /**< block memory for parameter settings */
   const char*           name,               /**< name of display column */
   const char*           desc,               /**< description of display column */
   const char*           header,             /**< head line of display column */
   SCIP_DISPSTATUS       dispstatus,         /**< display activation status of display column */
   SCIP_DECL_DISPCOPY    ((*dispcopy)),      /**< copy method of display column or NULL if you don't want to copy your plugin into subscips */
   SCIP_DECL_DISPFREE    ((*dispfree)),      /**< destructor of display column */
   SCIP_DECL_DISPINIT    ((*dispinit)),      /**< initialize display column */
   SCIP_DECL_DISPEXIT    ((*dispexit)),      /**< deinitialize display column */
   SCIP_DECL_DISPINITSOL ((*dispinitsol)),   /**< solving process initialization method of display column */
   SCIP_DECL_DISPEXITSOL ((*dispexitsol)),   /**< solving process deinitialization method of display column */
   SCIP_DECL_DISPOUTPUT  ((*dispoutput)),    /**< output method */
   SCIP_DISPDATA*        dispdata,           /**< display column data */
   int                   width,              /**< width of display column (no. of chars used) */
   int                   priority,           /**< priority of display column */
   int                   position,           /**< relative position of display column */
   SCIP_Bool             stripline           /**< should the column be separated with a line from its right neighbour? */
   );

/** frees memory of display column */
extern
SCIP_RETCODE SCIPdispFree(
   SCIP_DISP**           disp,               /**< pointer to display column data structure */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** initializes display column */
extern
SCIP_RETCODE SCIPdispInit(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** deinitializes display column */
extern
SCIP_RETCODE SCIPdispExit(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs display column that the branch and bound process is being started */
extern
SCIP_RETCODE SCIPdispInitsol(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** informs display column that the branch and bound process data is being freed */
extern
SCIP_RETCODE SCIPdispExitsol(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set                 /**< global SCIP settings */
   );

/** output display column to screen */
extern
SCIP_RETCODE SCIPdispOutput(
   SCIP_DISP*            disp,               /**< display column */
   SCIP_SET*             set,                /**< global SCIP settings */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** prints one line of output with the active display columns */
extern
SCIP_RETCODE SCIPdispPrintLine(
   SCIP_SET*             set,                /**< global SCIP settings */
   SCIP_STAT*            stat,               /**< problem statistics data */
   FILE*                 file,               /**< output file (or NULL for standard output) */
   SCIP_Bool             forcedisplay        /**< should the line be printed without regarding frequency? */
   );

/** activates all display lines fitting in the display w.r. to priority */
extern
SCIP_RETCODE SCIPdispAutoActivate(
   SCIP_SET*             set                 /**< global SCIP settings */
   );

#ifdef __cplusplus
}
#endif

#endif
