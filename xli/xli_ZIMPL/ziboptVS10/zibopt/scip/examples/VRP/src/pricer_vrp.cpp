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
/*  You should have received a copy of the ZIB Academic License.             */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: pricer_vrp.cpp,v 1.8 2010/09/13 15:29:28 bzfberth Exp $"

#include "pricer_vrp.h"
#include "pqueue.h"

#include <iostream>
#include <map>
#include <vector>

extern "C"
{
#include "scip/misc.h"
}

using namespace std;
using namespace scip;

#define SCIP_DEBUG 0

//------------------------------------------------------------
// Constructs the pricer object with the data needed
//------------------------------------------------------------
ObjPricerVRP::
ObjPricerVRP(
  SCIP*                               scip,
  const char*                         p_name,       // name of the pricer
  const int                           p_num_nodes,  // number of nodes (incl. depot)
  const int                           p_capacity,   // max demand of customers in tour 
  const vector<int>&                  p_demand,     // demand of each customer
  const vector<vector<int> >&         p_distance,   // distances between customers
  const vector< vector<SCIP_VAR*> >&  p_arc_var,    
  const vector< vector<SCIP_CONS*> >& p_arc_con,   
  const vector<SCIP_CONS*>&           p_part_con ):
   ObjPricer( scip,
              p_name, 
              "Finds tour with negative reduced cost.",
              0,
              TRUE) ,
   _num_nodes( p_num_nodes ),
   _capacity ( p_capacity ),
   _demand   ( p_demand ),
   _distance ( p_distance ),
   _arc_var  ( p_arc_var ),
   _arc_con  ( p_arc_con ),
   _part_con ( p_part_con )
{}

//------------------------------------------------------------
// Destructs the pricer object.
//------------------------------------------------------------
ObjPricerVRP::
~ObjPricerVRP() 
{}



//------------------------------------------------------------
// Initializes the pricer object.
// Because SCIP transformes the original problem in preprocessing, 
// we need to get the references to the variables and constraints
// in the transformed problem from the references in the original
// problem.
//------------------------------------------------------------
SCIP_RETCODE 
ObjPricerVRP::
scip_init 
( SCIP*              scip,               /**< SCIP data structure */
  SCIP_PRICER*       pricer )            /**< the variable pricer itself */
{
   for ( int i = 0; i < num_nodes(); ++i ) {
      for ( int j = 0; j < i; ++j ) {
         SCIPgetTransformedVar ( scip, _arc_var[i][j], &_arc_var[i][j] );
         SCIPgetTransformedCons( scip, _arc_con[i][j], &_arc_con[i][j] );
      }
   }
   for ( int i = 1; i < num_nodes(); ++i ) {
      SCIPgetTransformedCons( scip, _part_con[i], &_part_con[i] );
   }
   return SCIP_OKAY;
}



//------------------------------------------------------------
// Pricing of additional variables if LP is feasible:
// - get the values of the dual variables you need
// - construct the reduced-cost arc lengths from these values
// - find the shortest admissible tour with respect to these lengths
// - if this tour has negative reduced cost, add it to the LP
//------------------------------------------------------------
SCIP_RETCODE
ObjPricerVRP::
scip_redcost
( SCIP*              scip,               /**< SCIP data structure */
  SCIP_PRICER*       pricer,             /**< the variable pricer itself */
  SCIP_Real*         lowerbound,         /**< a lowerbound computed by the pricer */
  SCIP_RESULT*       result )            /**< the result of the pricer call */
{
#if ( SCIP_DEBUG >= 2 )
   cerr << "CALL scip_redcost" << endl;
#endif
   //--------------------
   // compute reduced-cost arc lengths
   // store only lower triangualar matrix, i.e., 
   // red_length[i][j] only for i > j
   //--------------------

   // set result pointer
   *result = SCIP_SUCCESS;

   // allocate array
   vector< vector<double> > red_length ( num_nodes() );
   for ( int i = 0; i < num_nodes(); ++i )
      red_length[i].resize( i, 0 );
   
   // compute reduced-cost lengths
   for ( int i = 0; i < num_nodes(); ++i ) {
      for ( int j = 0; j < i; ++j ) {
         red_length[i][j]  = 0;
         red_length[i][j] -= SCIPgetDualsolLinear( scip, arc_con(i,j) );
         if ( j != 0 )
            red_length[i][j] -= 0.5 * SCIPgetDualsolLinear( scip, part_con(j) );
         if ( i != 0 )
            red_length[i][j] -= 0.5 * SCIPgetDualsolLinear( scip, part_con(i) );
      }
   }
   
#if ( SCIP_DEBUG >= 3 )
   cout << "DUAL SOLUTION" << endl;
   for ( int i = 0; i < num_nodes(); ++i ) {
      for ( int j = 0; j < i; ++j ) {
         cout << "arc_" << i << "_" << j << "   "
              << SCIPgetDualsolLinear( scip, arc_con(i,j) ) << endl;
      }
   }
   for ( int i = 1; i < num_nodes(); ++i ) {
      cout << "part_" << i  << "   "
           << SCIPgetDualsolLinear( scip, part_con(i) ) << endl;
   }
   for ( int i = 0; i < num_nodes(); ++i ) {
      for ( int j = 0; j < i; ++j ) {
         cout << "length_" << i << "_" << j << "   " 
              << red_length[i][j] 
              << endl;
      }
   }

#endif
   //--------------------
   // compute shortest length restricted tour w.r.t. reduced-cost arc length
   //--------------------
   
   list<int> tour;
   double    reduced_cost;

   reduced_cost = find_shortest_tour( red_length, tour );
   
   //--------------------
   // add tour variable
   //--------------------
   if ( SCIPisNegative( scip, reduced_cost ) ) {
      return add_tour_variable( scip, tour ) ;
   }   

   return SCIP_OKAY;
}
  
//------------------------------------------------------------
// Pricing of additional variables if LP is infeasible.
//
// - get the values of the dual Farks multipliers you need
// - construct the reduced-cost arc lengths from these values
// - find the shortest admissible tour with respect to these lengths
// - if this tour has negative reduced cost, add it to the LP
//------------------------------------------------------------
SCIP_RETCODE
ObjPricerVRP::
scip_farkas
( SCIP*              scip,               /**< SCIP data structure */
  SCIP_PRICER*       pricer )            /**< the variable pricer itself */
{
#if ( SCIP_DEBUG >= 2 )
   cerr << "CALL scip_farkas" << endl;
#endif
   //--------------------
   // compute reduced-cost arc lengths
   // store only lower triangualar matrix, i.e., 
   // red_length[i][j] only for i > j
   //--------------------

   // allocate array
   vector< vector<double> > red_length ( num_nodes() );
   for ( int i = 0; i < num_nodes(); ++i )
      red_length[i].resize( i, 0 );
   
   // compute reduced-cost lengths
   for ( int i = 0; i < num_nodes(); ++i ) {
      for ( int j = 0; j < i; ++j ) {
         red_length[i][j]  = 0;
         red_length[i][j] -= SCIPgetDualfarkasLinear( scip, arc_con(i,j) );
         if ( j != 0 )
            red_length[i][j] -= 0.5 * SCIPgetDualfarkasLinear( scip, part_con(j) );
         if ( i != 0 )
            red_length[i][j] -= 0.5 * SCIPgetDualfarkasLinear( scip, part_con(i) );
      }
   }
   
   //--------------------
   // To Do: compute shortest length restricted tour w.r.t. duals
   //--------------------

   list<int> tour;
   double    reduced_cost;

   reduced_cost = find_shortest_tour( red_length, tour );
   
   if ( SCIPisNegative(scip, reduced_cost ) ) {
      return add_tour_variable( scip, tour ) ;
   }   

   return SCIP_OKAY;
}

//------------------------------------------------------------
// Adds a tour-variable to the LP
//------------------------------------------------------------
SCIP_RETCODE
ObjPricerVRP::
add_tour_variable
( SCIP*              scip,
  const list<int>&   tour ) 
{
   SCIP_VAR* var;
   char      var_name[255];
   char      tmp_name[255];
   
   // create meaningful variable name
   SCIPsnprintf(var_name, 255, "T" );
   for ( list<int>::const_iterator it = tour.begin(); it != tour.end(); ++it ) 
   {
      strncpy(tmp_name, var_name, 255);
      SCIPsnprintf(var_name, 255, "%s_%d", tmp_name, *it );
   }
   
#if ( SCIP_DEBUG >= 1 )
   cout << "NEW VARIABLE = " << var_name << endl;
#endif
   
   
   // create the new variable
   // Use upper bound of infinity such that we do not have
   // to care about the reduced costs of the variable in the
   // pricing. The upper bound of 1 is implicitly satisfied
   // due to the set partitioning constraints.
   SCIP_CALL( SCIPcreateVar(scip,
                            &var,                    // returns new index
                            var_name,                // name
                            0.0,                     // lower bound
                            SCIPinfinity(scip),      // upper bound
                            0,                       // objective
                            SCIP_VARTYPE_CONTINUOUS, // variable type
                            false,                   // forget the rest ...
                            false,                  
                            0,                      
                            0,
                            0,
                            0,
                            0 ) );
   
   // add new variable to the list of variables to price into LP
   SCIP_CALL( SCIPaddPricedVar(scip, 
                               var,             // new variable
                               1.0) );          // score: leave 1 here 
   
   // add coefficient into the set partition constraints
   for ( list<int>::const_iterator it = tour.begin(); 
         it != tour.end(); 
         ++it ) {
      SCIP_CALL( SCIPaddCoefLinear(scip,
                                   part_con(*it),
                                   var,
                                   1.0) );
   }

   // add coefficient into arc routing constraints
   int last = 0;
   for ( list<int>::const_iterator it = tour.begin(); 
         it != tour.end(); 
         ++it ) {
      SCIP_CALL( SCIPaddCoefLinear( scip, 
                                    arc_con(last,*it),
                                    var, 
                                    1.0 ) );
      last = *it;
   }
   SCIP_CALL( SCIPaddCoefLinear( scip, 
                                 arc_con(last,0), 
                                 var, 
                                 1.0 ) );   
   
   // cleanup
   SCIP_CALL( SCIPreleaseVar(scip, &var) );
   
   return SCIP_OKAY;
}


//------------------------------------------------------------
// Computes a shortest admissible tour with respect to the given
// lengths. 
// The function must returns the computed tour via the parameter 
// tour and the length (w.r.t. given lengths) of this tour as 
// return parameter. The returned tour must be the ordered list 
// of customer nodes contained in the tour (i.e., 2-5-7 for the tour 
// 0-2-5-7-0).
//------------------------------------------------------------
namespace {
//--------------------
// types needed for prioity queue
//--------------------
static const double eps = 1e-10;

struct PQUEUE_KEY {
   int    demand;
   double length;
   PQUEUE_KEY() : demand( 0 ), length( 0 ) {}
};
bool operator< ( const PQUEUE_KEY& l1, const PQUEUE_KEY& l2 ) {
   if ( l1.demand < l2.demand )     return true;
   if ( l1.demand > l2.demand )     return false;
   if ( l1.length < l2.length-eps ) return true;
   if ( l1.length > l2.length+eps ) return false;
   return false;
}
typedef int                                    PQUEUE_DATA; // node
typedef pqueue<PQUEUE_KEY,PQUEUE_DATA>         PQUEUE;
typedef PQUEUE::pqueue_item                    PQUEUE_ITEM;

//--------------------
// types needed for dyn. programming table
//--------------------
struct NODE_TABLE_DATA {
   double                length;
   int                   predecessor;
   PQUEUE::pqueue_item   queue_item;
   NODE_TABLE_DATA( ) : length( 0 ), predecessor( -1 ), queue_item( NULL ) {}
};

typedef int NODE_TABLE_KEY; // demand
typedef std::map< NODE_TABLE_KEY, NODE_TABLE_DATA >   NODE_TABLE;
}
//--------------------
double
ObjPricerVRP::
find_shortest_tour
( const vector< vector<double> >& length,
  list<int>&                      tour )
{
   tour.clear();

#if ( SCIP_DEBUG >= 2 )
   cerr << "Enter RSP " << capacity() << endl;
#endif

   // begin algorithm
   PQUEUE                    PQ;
   vector< NODE_TABLE >      table ( num_nodes() );
   
   // insert root node ( start at node 0 )
   PQUEUE_KEY       queue_key;
   PQUEUE_DATA      queue_data = 0;
   PQUEUE_ITEM      queue_item = PQ.insert( queue_key, queue_data ); 
   
   NODE_TABLE_KEY   table_key = 0;
   NODE_TABLE_DATA  table_entry; 
   
   // run Dijkstra-like updates
   while ( PQ.empty() == false ) {
      
      // get topmost queue entry
      queue_item = PQ.top();
      queue_key  = PQ.get_key ( queue_item );
      queue_data = PQ.get_data( queue_item );
      PQ.pop();
            
      // get corresponding node and node-table key
      const int    curr_node   = queue_data;
      const double curr_length = queue_key.length;
      const int    curr_demand = queue_key.demand;

      // stop as soon as some negative length tour was found
      if ( curr_node == 0 && curr_length < -eps )
         break;

      // stop as soon don't create multi-tours 
      if ( curr_node == 0 && curr_demand != 0 )
         continue;
      
      // update all active neighbors
      for( int next_node = 0; next_node < num_nodes(); ++next_node ) {
         if ( next_node == curr_node )
            continue;         
         if ( have_edge( next_node, curr_node ) == false )
            continue;
         
         const int    next_demand = curr_demand + demand(next_node);
         const double next_length = curr_length + ( curr_node > next_node ? 
                                                    length[curr_node][next_node] :
                                                    length[next_node][curr_node] );
         if ( next_demand > capacity() )
            continue;
         
         NODE_TABLE& next_table = table[next_node];

         // check if new table entry would be dominated
         bool                       skip      = false;
         list<NODE_TABLE::iterator> dominated;
         for ( NODE_TABLE::iterator it = next_table.begin();
               it != next_table.end() && skip == false;
               ++it ) {
            if ( next_demand >= it->first &&
                 next_length >= it->second.length - eps  ) {
                  skip = true;
            }
            if ( next_demand <= it->first &&
                 next_length <= it->second.length + eps ) {
               dominated.push_front( it );
            }
         }
         if ( skip ) {
            continue;
         }
         // remove dominated table and queue entries
         for ( list<NODE_TABLE::iterator>::iterator it = dominated.begin();
               it != dominated.end();
               ++it ) {
            PQ.remove( (*it)->second.queue_item );
            next_table.erase( *it );
         }
         
         // insert new table and queue entry 
         queue_key.demand = next_demand;
         queue_key.length = next_length;
         queue_data       = next_node;
         
         queue_item = PQ.insert( queue_key, queue_data );

         table_key               = next_demand;                  
         table_entry.length      = next_length;
         table_entry.predecessor = curr_node;
         table_entry.queue_item  = queue_item;
         
         next_table[table_key] = table_entry;         
#if ( SCIP_DEBUG >= 2 )
         cerr << "NEW ENTRY "
              << " node=" << next_node
              << " demand=" << next_demand 
              << " length=" << next_length
              << " pred=" << curr_node 
              << endl;
#endif
         
      }
   }
   
#if ( SCIP_DEBUG >= 2 )
   cout << "Done RSP DP" << endl;
#endif

   table_entry.predecessor = -1;
   table_entry.length      = 0;
   int curr_node = 0;
   // find most negative tour
   for ( NODE_TABLE::iterator it = table[0].begin();
         it != table[0].end();
         ++it ) {
      if ( it->second.length < table_entry.length ) {
         table_key   = it->first;
         table_entry = it->second;
      }
   }
   double tour_length = table_entry.length;
   
   while ( table_entry.predecessor > 0 ) {
      table_key -= demand(curr_node);
      curr_node  = table_entry.predecessor;
      tour.push_front( curr_node );
      table_entry = table[curr_node][table_key];
   }
   
#if ( SCIP_DEBUG >= 2 )
   cerr << "Leave RSP " << tour_length << endl;
#endif
   
   return tour_length;
}

