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
#pragma ident "@(#) $Id: nodesel_dfs.c,v 1.37 2010/09/27 17:20:23 bzfheinz Exp $"

/**@file   nodesel_dfs.c
 * @ingroup NODESELECTORS
 * @brief  node selector for depth first search
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

#include "scip/nodesel_dfs.h"


#define NODESEL_NAME             "dfs"
#define NODESEL_DESC             "depth first search"
#define NODESEL_STDPRIORITY           0
#define NODESEL_MEMSAVEPRIORITY  100000




/*
 * Callback methods
 */

/** copy method for node selector plugins (called when SCIP copies plugins) */
static
SCIP_DECL_NODESELCOPY(nodeselCopyDfs)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);

   /* call inclusion method of node selector */
   SCIP_CALL( SCIPincludeNodeselDfs(scip) );

   return SCIP_OKAY;
}


/** destructor of node selector to free user data (called when SCIP is exiting) */
#define nodeselFreeDfs NULL


/** initialization method of node selector (called after problem was transformed) */
#define nodeselInitDfs NULL


/** deinitialization method of node selector (called before transformed problem is freed) */
#define nodeselExitDfs NULL


/** solving process initialization method of node selector (called when branch and bound process is about to begin) */
#define nodeselInitsolDfs NULL


/** solving process deinitialization method of node selector (called before branch and bound process data is freed) */
#define nodeselExitsolDfs NULL


/** node selection method of node selector */
static
SCIP_DECL_NODESELSELECT(nodeselSelectDfs)
{  /*lint --e{715}*/
   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);
   assert(selnode != NULL);

   *selnode = SCIPgetPrioChild(scip);
   if( *selnode == NULL )
   {
      *selnode = SCIPgetPrioSibling(scip);
      if( *selnode == NULL )
      {
         SCIPdebugMessage("select best leaf");
         *selnode = SCIPgetBestLeaf(scip);
      }

      SCIPdebugMessage("select best sibling leaf");
   }

   return SCIP_OKAY;
}


/** node comparison method of node selector */
static
SCIP_DECL_NODESELCOMP(nodeselCompDfs)
{  /*lint --e{715}*/
   int depth1;
   int depth2;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   depth1 = SCIPnodeGetDepth(node1);
   depth2 = SCIPnodeGetDepth(node2);
   if( depth1 > depth2 )
      return -1;
   else if( depth1 < depth2 )
      return +1;
   else
   {
      SCIP_Real lowerbound1;
      SCIP_Real lowerbound2;

      lowerbound1 = SCIPnodeGetLowerbound(node1);
      lowerbound2 = SCIPnodeGetLowerbound(node2);
      if( lowerbound1 < lowerbound2 )
         return -1;
      else if( lowerbound1 > lowerbound2 )
         return +1;
      else
         return 0;
   }
}





/*
 * dfs specific interface methods
 */

/** creates the node selector for depth first search and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodeselDfs(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODESELDATA* nodeseldata;

   /* create dfs node selector data */
   nodeseldata = NULL;

   /* include node selector */
   SCIP_CALL( SCIPincludeNodesel(scip, NODESEL_NAME, NODESEL_DESC, NODESEL_STDPRIORITY, NODESEL_MEMSAVEPRIORITY,
         nodeselCopyDfs,
         nodeselFreeDfs, nodeselInitDfs, nodeselExitDfs, 
         nodeselInitsolDfs, nodeselExitsolDfs, nodeselSelectDfs, nodeselCompDfs,
         nodeseldata) );

   return SCIP_OKAY;
}

