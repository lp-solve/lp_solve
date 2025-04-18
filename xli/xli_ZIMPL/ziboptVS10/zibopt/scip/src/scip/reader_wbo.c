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
#pragma ident "@(#) $Id: reader_wbo.c,v 1.3 2010/09/27 17:20:24 bzfheinz Exp $"

/**@file   reader_wbo.c
 * @ingroup FILEREADERS 
 * @brief  WBO file reader (OPB format with weighted constraints)
 * @author Michael Winkler
 */

/* For file format description see opb-reader. */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <string.h>

#include "scip/reader_opb.h"
#include "scip/reader_wbo.h"

#define READER_NAME             "wboreader"
#define READER_DESC             "file reader for pseudoboolean wbo file format"
#define READER_EXTENSION        "wbo"

/*
 * Callback methods of reader
 */

/** copy method for reader plugins (called when SCIP copies plugins) */
static
SCIP_DECL_READERCOPY(readerCopyWbo)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(reader != NULL);
   assert(strcmp(SCIPreaderGetName(reader), READER_NAME) == 0);

   /* call inclusion method of reader */
   SCIP_CALL( SCIPincludeReaderWbo(scip) );
 
   return SCIP_OKAY;
}


/** destructor of reader to free user data (called when SCIP is exiting) */
#define readerFreeWbo NULL


/** problem reading method of reader */
static
SCIP_DECL_READERREAD(readerReadWbo)
{  /*lint --e{715}*/

   SCIP_CALL( SCIPreadOpb(scip, reader, filename, result) );

   return SCIP_OKAY;
}


/** problem writing method of reader */
static
SCIP_DECL_READERWRITE(readerWriteWbo)
{  /*lint --e{715}*/
   SCIP_CALL( SCIPwriteOpb(scip, file, name, transformed, objsense, objscale, objoffset, vars,
         nvars, nbinvars, nintvars, nimplvars, ncontvars, nfixedvars, conss, nconss, genericnames, result) );

   return SCIP_OKAY;
}


/*
 * reader specific interface methods
 */

/** includes the wbo file reader in SCIP */
SCIP_RETCODE SCIPincludeReaderWbo(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_READERDATA* readerdata;

   /* create lp reader data */
   readerdata = NULL;

   /* include lp reader */
   SCIP_CALL( SCIPincludeReader(scip, READER_NAME, READER_DESC, READER_EXTENSION,
         readerCopyWbo,
         readerFreeWbo, readerReadWbo, readerWriteWbo,
         readerdata) );

   return SCIP_OKAY;
}
