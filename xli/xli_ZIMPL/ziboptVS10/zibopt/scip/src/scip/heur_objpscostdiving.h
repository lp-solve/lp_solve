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
#pragma ident "@(#) $Id: heur_objpscostdiving.h,v 1.14 2010/01/04 20:35:40 bzfheinz Exp $"

/**@file   heur_objpscostdiving.h
 * @brief  LP diving heuristic that changes variable's objective value instead of bounds, using pseudo cost values as guide
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_HEUR_OBJPSCOSTDIVING_H__
#define __SCIP_HEUR_OBJPSCOSTDIVING_H__


#include "scip/scip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates the objpscostdiving heuristic and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludeHeurObjpscostdiving(
   SCIP*                 scip                /**< SCIP data structure */
   );

#ifdef __cplusplus
}
#endif

#endif
