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
#pragma ident "@(#) $Id: pricer_binpacking.h,v 1.1 2010/09/09 12:21:14 bzfheinz Exp $"

/**@file   pricer_binpacking.h
 * @brief  binpacking variable pricer
 * @author Timo Berthold
 * @author Stefan Heinz
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __BIN_PRICER_BINPACKING__
#define __BIN_PRICER_BINPACKING__

#include "scip/scip.h"


/** creates the binpacking variable pricer and includes it in SCIP */
extern
SCIP_RETCODE SCIPincludePricerBinpacking(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** added problem specific data to pricer and activates pricer */
extern
SCIP_RETCODE SCIPpricerBinpackingActivate(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           conss,              /**< set covering constraints for the items */
   SCIP_Longint*         weights,            /**< weight of the items */
   int*                  ids,                /**< array of item ids */                
   int                   nitems,             /**< number of items to be packed */
   SCIP_Longint          capacity            /**< capacity of the bins */
   );

#endif
