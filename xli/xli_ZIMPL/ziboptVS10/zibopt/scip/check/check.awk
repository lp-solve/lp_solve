#!/usr/bin/awk -f
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                           *
#*                  This file is part of the program and library             *
#*         SCIP --- Solving Constraint Integer Programs                      *
#*                                                                           *
#*    Copyright (C) 2002-2010 Konrad-Zuse-Zentrum                            *
#*                            fuer Informationstechnik Berlin                *
#*                                                                           *
#*  SCIP is distributed under the terms of the ZIB Academic License.         *
#*                                                                           *
#*  You should have received a copy of the ZIB Academic License              *
#*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      *
#*                                                                           *
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# $Id: check.awk,v 1.101 2010/09/20 16:11:19 bzfschwa Exp $
#
#@file    check.awk
#@brief   SCIP Check Report Generator
#@author  Thorsten Koch
#@author  Tobias Achterberg
#@author  Alexander Martin
#@author  Timo Berthold
#@author  Robert Waniek
#@author  Gregor Hendel
#
function abs(x)
{
   return x < 0 ? -x : x;
}
function min(x,y)
{
   return (x) < (y) ? (x) : (y);
}
function max(x,y)
{
   return (x) > (y) ? (x) : (y);
}
BEGIN {
   timegeomshift = 10.0;
   nodegeomshift = 100.0;
   sblpgeomshift = 0.0;
   pavshift = 1.0;
   onlyinsolufile = 0;          # should only instances be reported that are included in the .solu file?
   onlyintestfile = 0;          # should only instances be reported that are included in the .test file?  TEMPORARY HACK!
   onlypresolvereductions = 0;  # should only instances with presolve reductions be shown?
   useshortnames = 1;           # should problem name be truncated to fit into column?
   writesolufile = 0;           # should a solution file be created from the results
   printsoltimes = 0;           # should the times until first and best solution be shown
   NEWSOLUFILE = "new_solufile.solu";
   infty = +1e+20;
   headerprinted = 0;

   nprobs = 0;
   sbab = 0;
   slp = 0;
   ssim = 0;
   ssblp = 0;
   stottime = 0.0;
   stimetofirst = 0.0;
   stimetobest = 0.0;
   nodegeom = 0.0;
   timegeom = 0.0;
   timetofirstgeom = 0.0;
   timetobestgeom = 0.0;
   sblpgeom = 0.0;
   conftimegeom = 0.0;
   basictimegeom = 0.0;
   overheadtimegeom = 0.0;
   shiftednodegeom = nodegeomshift;
   shiftedtimegeom = timegeomshift;
   shiftedsblpgeom = sblpgeomshift;
   shiftedconftimegeom = timegeomshift;
   shiftedbasictimegeom = timegeomshift;
   shiftedoverheadtimegeom = timegeomshift;
   shiftedtimetofirstgeom = timegeomshift;
   shiftedtimetobestgeom = timegeomshift;
   timeouttime = 0.0;
   timeouts = 0;
   failtime = 0.0;
   fail = 0;
   pass = 0;
   settings = "default";
   lpsname = "?";
   lpsversion = "?";
   scipversion = "?";
   conftottime = 0.0;
   overheadtottime = 0.0;
}
/^IP\// {  # TEMPORARY HACK to parse .test files
   intestfile[$1] = 1;
}
/=opt=/  { solstatus[$2] = "opt"; sol[$2] = $3; }   # get optimum
/=inf=/  { solstatus[$2] = "inf"; }                 # problem infeasible (no feasible solution exists)
/=best=/ { solstatus[$2] = "best"; sol[$2] = $3; }  # get best known solution value
/=feas=/ { solstatus[$2] = "feas"; }                # no feasible solution known
/=unkn=/ { solstatus[$2] = "unkn"; }                # no feasible solution known
#
# problem name
#
/^@01/ { 
   filename = $2;

   n  = split ($2, a, "/");
   m = split(a[n], b, ".");
   prob = b[1];
   if( b[m] == "gz" || b[m] == "z" || b[m] == "GZ" || b[m] == "Z" )
      m--;
   for( i = 2; i < m; ++i )
      prob = prob "." b[i];

   if( useshortnames && length(prob) > 18 )
      shortprob = substr(prob, length(prob)-17, 18);
   else
      shortprob = prob;

   # Escape _ for TeX
   n = split(prob, a, "_");
   pprob = a[1];
   for( i = 2; i <= n; i++ )
      pprob = pprob "\\_" a[i];
   vars = 0;
   binvars = 0;
   intvars = 0;
   implvars = 0;
   contvars = 0;
   cons = 0;
   lincons = 0;
   origvars = 0;
   origcons = 0;
   timeout = 0;
   feasible = 0;
   pb = +infty;
   firstpb = +infty;
   db = -infty;
   rootdb = -infty;
   simpiters = 0;
   bbnodes = 0;
   primlps = 0;
   primiter = 0;
   duallps = 0;
   dualiter = 0;
   sblps = 0;
   sbiter = 0;
   tottime = 0.0;
   timetofirst = -1.0;
   timetobest = -1.0;
   inconflict = 0;
   inconstime = 0;
   confclauses = 0;
   confliterals = 0.0;
   conftime = 0.0;
   overheadtime = 0.0;
   aborted = 1;
   readerror = 0;
   gapreached = 0;
   sollimitreached = 0;
   memlimitreached = 0;
   starttime = 0.0;
   endtime = 0.0;
   timelimit = 0.0;
   inoriginalprob = 1;
   incons = 0;
}

/@03/ { starttime = $2; }
/@04/ { endtime = $2; }

/^SCIP version/ {
   # get SCIP version 
   scipversion = $3; 

   # get name of LP solver
   if( $13 == "SoPlex" )
      lpsname = "spx";
   else if( $13 == "CPLEX" )
      lpsname = "cpx";
   else if( $13 == "NONE" )
      lpsname = "none";
   else if( $13 == "Clp" )
      lpsname = "clp";
   else if( $13 == "MOSEK" )
      lpsname = "msk";
   else if( $13 == "Gurobi" )
      lpsname = "grb";
   else if( $13 == "NONE" )
      lpsname = "none";
   else if( $13 == "QSopt" )
      lpsname = "qso";
#   else if( $13 == "???" )
#      lpsname = "xprs";

    # get LP solver version 
   if( NF >= 14 ) {
      split($14, v, "]");
      lpsversion = v[1];
   }
}
/^SCIP> SCIP> / { $0 = substr($0, 13, length($0)-12); }
/^SCIP> / { $0 = substr($0, 7, length($0)-6); }
/^loaded parameter file/ { settings = $4; sub(/<.*settings\//, "", settings); sub(/\.set>/, "", settings); }
/^parameter <limits\/time> set to/ { timelimit = $5; }
/^limits\/time =/ { timelimit = $3; }
#
# conflict analysis
#
/^Conflict Analysis  :/ { inconflict = 1; }
/^  propagation      :/ {
   if( inconflict == 1 ) {
      conftime += $3; #confclauses += $5 + $7; confliterals += $5 * $6 + $7 * $8;
   }
}
/^  infeasible LP    :/ {
   if( inconflict == 1 ) {
      conftime += $4; #confclauses += $6 + $8; confliterals += $6 * $7 + $8 * $9;
   }
}
/^  strong branching :/ {
   if( inconflict == 1 ) {
      conftime += $4; #confclauses += $6 + $8; confliterals += $6 * $7 + $8 * $9;
   }
}
/^  pseudo solution  :/ {
   if( inconflict == 1 ) {
      conftime += $4; #confclauses += $6 + $8; confliterals += $6 * $7 + $8 * $9;
   }
}
/^  applied globally :/ {
   if( inconflict == 1 ) {
      confclauses += $7; confliterals += $7 * $8;
   }
}
/^  applied locally  :/ {
   if( inconflict == 1 ) {
      confclauses += $7; confliterals += $7 * $8;
   }
}
/^Separators         :/ { inconflict = 0; }
/^Constraint Timings :/ { inconstime = 1; }
#/^  logicor          :/ { if( inconstime == 1 ) { overheadtime += $3; } }
/^Propagators        :/ { inconstime = 0; }
/^  switching time   :/ { overheadtime += $4; }
#
# problem size
#
/^Presolved Problem  :/ { inoriginalprob = 0; }
/^  Variables        :/ {
   if( inoriginalprob )
      origvars = $3;
   else {
      vars = $3;
      intvars = $6;
      implvars = $8;
      contvars = $11;
      binvars = vars - intvars - implvars - contvars;
   }
}
/^  Constraints      :/ {
   if( inoriginalprob )
      origcons = $3;
   else
      cons = $3;
}

#
# count number of linear constraints 
#
/^Constraints        :/ {
   incons = 1;
   lincons = 0;
}
/^  knapsack         :/ {
   if( incons == 1 ) {
      n  = split ($3, a, "+");
      lincons += a[1];
   }
}
/^  setppc           :/ {
   if( incons == 1 ) {
      n  = split ($3, a, "+");
      lincons += a[1];
   }
}
/^  linear           :/ { 
   if( incons  == 1 ) {
      n  = split ($3, a, "+");
      lincons += a[1];
   }
}
/^  logicor          :/ { 
   if( incons == 1 ) {
      n  = split ($3, a, "+");
      lincons += a[1];
   }
}
/^  varbound         :/ { 
   if( incons == 1 ) {
      n  = split ($3, a, "+");
      lincons += a[1];
   }
}
/^Constraint Timings :/ {
   incons = 0;
}

#
# solution
#
/^Original Problem   : no problem exists./ { readerror = 1; }
/^SCIP Status        :/ { aborted = 0; }
/solving was interrupted/ { timeout = 1; }
/gap limit reached/ { gapreached = 1; }
/solution limit reached/ { sollimitreached = 1; }
/memory limit reached/ { memlimitreached = 1; }
/problem is solved/ { timeout = 0; }
/^  First Solution   :/ {
   timetofirst = $11;
   firstpb = $4;
}
/^  Primal Bound     :/ {
   if( $4 == "infeasible" ) {
      pb = +infty;
      db = +infty;
      feasible = 0;
   }
   else if( $4 == "-" ) {
      pb = +infty;
      feasible = 0;
   }
   else {
      pb = $4;
      feasible = 1;
      timetobest = $11;
   }
}
/^  Dual Bound       :/ { 
   if( $4 != "-" ) 
      db = $4;
}
/^  Root Dual Bound  :/ {
   if( $5 != "-" )
      rootdb = $5;
}
#
# iterations
#
/^  primal LP        :/ { simpiters += $6; }
/^  dual LP          :/ { simpiters += $6; }
/^  barrier LP       :/ { simpiters += $6; }
/^  nodes \(total\)    :/ { bbnodes = $4 }
/^  primal LP        :/ { primlps = $5; primiter = $6; }
/^  dual LP          :/ { duallps = $5; dualiter = $6; }
/^  strong branching :/ { sblps = $5; sbiter = $6; }
#
# time
#
/^Solving Time       :/ { tottime = $4 }
#
# solver status overview (in order of priority): 
# 1) solver broke before returning solution => abort
# 2) solver cut off the optimal solution (solu-file-value is not between primal and dual bound) => fail
#    (especially of problem is claimed to be solved but solution is not the optimal solution)
# 3) solver solved problem with the value in solu-file (if existing) => ok
# 4) solver solved problem which has no (optimal) value in solu-file => solved
#    (since we here don't detect the direction of optimization, it is possible 
#     that a solver claims an optimal solution which contradicts a known feasible solution)
# 5) solver found solution better than known best solution (or no solution was known so far) => better
# 7) solver reached gaplimit or limit of number of solutions => gaplimit, sollimit
# 8) solver reached any other limit (like time or nodes) => timeout
# 9) otherwise => unknown
#
/^=ready=/ {

   #since the header depends on the parameter printsoltimes and settings it is no longer possible to print it in the BEGIN section
   if( !headerprinted ) {
      ntexcolumns = 8 + (2 * printsoltimes);
      
      #print header of tex file table
      printf("\\documentclass[leqno]{article}\n")                      >TEXFILE;
      printf("\\usepackage{a4wide}\n")                                 >TEXFILE;
      printf("\\usepackage{amsmath,amsfonts,amssymb,booktabs}\n")      >TEXFILE;
      printf("\\usepackage{supertabular}\n")                           >TEXFILE;
      printf("\\pagestyle{empty}\n\n")                                 >TEXFILE;
      printf("\\begin{document}\n\n")                                  >TEXFILE;
      printf("\\begin{center}\n")                                      >TEXFILE;
      printf("\\setlength{\\tabcolsep}{2pt}\n")                        >TEXFILE;
      printf("\\newcommand{\\g}{\\raisebox{0.25ex}{\\tiny $>$}}\n")    >TEXFILE;
      printf("\\tablehead{\n\\toprule\n")                              >TEXFILE;
      printf("Name                &  Conss &   Vars &     Dual Bound &   Primal Bound &  Gap\\%% &     Nodes &     Time ") >TEXFILE;
      if( printsoltimes )
         printf(" &     To First      &    To Last   ") > TEXFILE;
      printf("\\\\\n") > TEXFILE;
      printf("\\midrule\n}\n")                                         >TEXFILE;
      printf("\\tabletail{\n\\midrule\n")                              >TEXFILE;
      printf("\\multicolumn{%d}{r} \\; continue next page \\\\\n", ntexcolumns) >TEXFILE;
      printf("\\bottomrule\n}\n")                                      >TEXFILE;
      printf("\\tablelasttail{\\bottomrule}\n")                        >TEXFILE;
      printf("\\tablecaption{SCIP with %s settings}\n",settings)       >TEXFILE;
      printf("\\begin{supertabular*}{\\textwidth}{@{\\extracolsep{\\fill}}lrrrrrrr") >TEXFILE;
      if( printsoltimes )
         printf("rr") > TEXFILE;
      printf("@{}}\n") > TEXFILE;
      
      #print header of table when this regular expression is matched for the first time
      tablehead1 = "------------------+------+--- Original --+-- Presolved --+----------------+----------------+------+--------+-------+-------+";
      tablehead2 = "Name              | Type | Conss |  Vars | Conss |  Vars |   Dual Bound   |  Primal Bound  | Gap%% |  Iters | Nodes |  Time |";
      tablehead3 = "------------------+------+-------+-------+-------+-------+----------------+----------------+------+--------+-------+-------+";
      
      if( printsoltimes == 1 ) {  
         tablehead1 = tablehead1"----------+---------+";
         tablehead2 = tablehead2" To First | To Best |";
         tablehead3 = tablehead3"----------+---------+";
      } 
      
      tablehead1 = tablehead1"--------\n";
      tablehead2 = tablehead2"       \n";
      tablehead3 = tablehead3"--------\n";
   
      printf(tablehead1);
      printf(tablehead2);
      printf(tablehead3);

      headerprinted = 1;
   }

   if( (!onlyinsolufile || solstatus[prob] != "") &&
       (!onlyintestfile || intestfile[filename]) ) {

      #avoid problems when comparing floats and integer (make everything float)
      temp = pb;
      pb = 1.0*temp;
      temp = db;
      db = 1.0*temp;
      
      #firstpb and rootdb are used to detect the direction of optimization (min or max)
      if( timetofirst < 0.0 )
         temp = pb;
      else
         temp = firstpb;
      firstpb = 1.0*temp;
      temp = rootdb;
      rootdb = 1.0*temp;

      nprobs++;

      optimal = 0;
      markersym = "\\g";
      if( abs(pb - db) < 1e-06 && pb < infty ) {
         gap = 0.0;
         optimal = 1;
         markersym = "  ";
      }
      else if( abs(db) < 1e-06 )
         gap = -1.0;
      else if( pb*db < 0.0 )
         gap = -1.0;
      else if( abs(db) >= +infty )
         gap = -1.0;
      else if( abs(pb) >= +infty )
         gap = -1.0;
      else
         gap = 100.0*abs((pb-db)/db);
      if( gap < 0.0 )
         gapstr = "  --  ";
      else if( gap < 1e+04 )
         gapstr = sprintf("%6.1f", gap);
      else
         gapstr = " Large";
      
      if( vars == 0 )
         probtype = "--";
      else if( lincons < cons )
         probtype = "CIP";
      else if( binvars == 0 && intvars == 0 )
         probtype = "LP";
      else if( contvars == 0 ) {
         if( intvars == 0 && implvars == 0 )
            probtype = "BP";
         else
            probtype = "IP";
      }
      else {
         if( intvars == 0 )
            probtype = "MBP";
         else
            probtype = "MIP";
      }

      if( aborted && endtime - starttime > timelimit && timelimit > 0.0 ) {
         timeout = 1;
         aborted = 0;
         tottime = endtime - starttime;
      }
      else if( gapreached || sollimitreached )
         timeout = 0;
      if( aborted && tottime == 0.0 )
         tottime = timelimit;
      if( timelimit > 0.0 )
         tottime = min(tottime, timelimit);

      if( aborted || timetobest < 0.0 ) {
         timetofirst = tottime;
         timetobest = tottime;
      }

      lps = primlps + duallps;
      simplex = primiter + dualiter;
      stottime += tottime;
      stimetofirst += timetofirst;
      stimetobest += timetobest;
      sbab += bbnodes;
      slp += lps;
      ssim += simplex;
      ssblp += sblps;
      conftottime += conftime;
      overheadtottime += overheadtime;
      basictime = tottime - conftime - overheadtime;

      nodegeom = nodegeom^((nprobs-1)/nprobs) * max(bbnodes, 1.0)^(1.0/nprobs);
      sblpgeom = sblpgeom^((nprobs-1)/nprobs) * max(sblps, 1.0)^(1.0/nprobs);
      timegeom = timegeom^((nprobs-1)/nprobs) * max(tottime, 1.0)^(1.0/nprobs);
      conftimegeom = conftimegeom^((nprobs-1)/nprobs) * max(conftime, 1.0)^(1.0/nprobs);
      overheadtimegeom = overheadtimegeom^((nprobs-1)/nprobs) * max(overheadtime, 1.0)^(1.0/nprobs);
      basictimegeom = basictimegeom^((nprobs-1)/nprobs) * max(basictime, 1.0)^(1.0/nprobs);

      shiftednodegeom = shiftednodegeom^((nprobs-1)/nprobs) * max(bbnodes+nodegeomshift, 1.0)^(1.0/nprobs);
      shiftedsblpgeom = shiftedsblpgeom^((nprobs-1)/nprobs) * max(sblps+sblpgeomshift, 1.0)^(1.0/nprobs);
      shiftedtimegeom = shiftedtimegeom^((nprobs-1)/nprobs) * max(tottime+timegeomshift, 1.0)^(1.0/nprobs);
      shiftedconftimegeom = shiftedconftimegeom^((nprobs-1)/nprobs) * max(conftime+timegeomshift, 1.0)^(1.0/nprobs);
      shiftedoverheadtimegeom = shiftedoverheadtimegeom^((nprobs-1)/nprobs) * max(overheadtime+timegeomshift, 1.0)^(1.0/nprobs);
      shiftedbasictimegeom = shiftedbasictimegeom^((nprobs-1)/nprobs) * max(basictime+timegeomshift, 1.0)^(1.0/nprobs);

      timetobestgeom = timetobestgeom^((nprobs-1)/nprobs) * max(timetobest,1.0)^(1.0/nprobs);
      timetofirstgeom = timetofirstgeom^((nprobs-1)/nprobs) * max(timetofirst,1.0)^(1.0/nprobs);
      shiftedtimetofirstgeom = shiftedtimetofirstgeom^((nprobs-1)/nprobs) * max(timetofirst + timegeomshift, 1.0)^(1.0/nprobs);
      shiftedtimetobestgeom = shiftedtimetobestgeom^((nprobs-1)/nprobs) * max(timetobest + timegeomshift, 1.0)^(1.0/nprobs);

      status = "";
      if( readerror ) {
         status = "readerror";
         failtime += tottime;
         fail++;
      }
      else if( aborted ) {
         status = "abort";
         failtime += tottime;
         fail++;
      }
      else if( solstatus[prob] == "opt" ) {
         reltol = 1e-5 * max(abs(pb),1.0);
         abstol = 1e-4;

         if( ( firstpb-rootdb > max(abstol,reltol) && (db-sol[prob] > reltol || sol[prob]-pb > reltol) ) || ( rootdb-firstpb > max(reltol,abstol) && (sol[prob]-db > reltol || pb-sol[prob] > reltol) ) ) {
            status = "fail";
            failtime += tottime;
            fail++;
         }
         else {
            if( timeout || gapreached || sollimitreached ) {
               if( timeout )
                  status = "timeout";
               else if( gapreached )
                  status = "gaplimit";
               else if( sollimitreached )
                  status = "sollimit";
               timeouttime += tottime;
               timeouts++;
            }
            else {
               if( (abs(pb - db) <= max(abstol, reltol)) && abs(pb - sol[prob]) <= reltol ) {
                  status = "ok";
                  pass++;
               }
               else {
                  status = "fail";
                  failtime += tottime;
                  fail++;
               }
            }
         }
      }
      else if( solstatus[prob] == "best" ) {
         reltol = 1e-5 * max(abs(pb),1.0);
         abstol = 1e-4;

         if( ( firstpb-rootdb > max(abstol,reltol) && db-sol[prob] > reltol) || ( rootdb-firstpb > max(reltol,abstol) && sol[prob]-db > reltol) ) {
            status = "fail";
            failtime += tottime;
            fail++;
         }
         else {
            if( timeout || gapreached || sollimitreached ) {
               if( (firstpb-rootdb > max(abstol,reltol) && sol[prob]-pb > reltol) || (rootdb-firstpb > max(abstol,reltol) && pb-sol[prob] > reltol) ) {
                  status = "better";
                  timeouttime += tottime;
                  timeouts++;
               }
               else {
                  if( timeout )
                     status = "timeout";
                  else if( gapreached )
                     status = "gaplimit";
                  else if( sollimitreached )
                     status = "sollimit";
                  timeouttime += tottime;
                  timeouts++;
               }
            }
            else {
               if( abs(pb - db) <= max(abstol, reltol) ) {
                  if( abs(firstpb - rootdb) <= max(abstol,reltol) )
                     status = "solved not verified";
                  else
                     status = "solved";
                  pass++;
               }
               else {
                  status = "fail";
                  failtime += tottime;
                  fail++;
               }
            }
         }
      }
      else if( solstatus[prob] == "unkn" ) {
         reltol = 1e-5 * max(abs(pb),1.0);
         abstol = 1e-4;
         
         if( abs(pb - db) <= max(abstol, reltol) ) {
            status = "solved not verified";
            pass++;
         }
         else {
            if( abs(pb) < infty ) {
               status = "better";
               timeouttime += tottime;
               timeouts++;
            }
            else {
               if( timeout || gapreached || sollimitreached ) {
                  if( timeout )
                     status = "timeout";
                  else if( gapreached )
                     status = "gaplimit";
                  else if( sollimitreached )
                     status = "sollimit";
                  timeouttime += tottime;
                  timeouts++;
               }
               else 
                  status = "unknown";
            }
         }
      }
      else if( solstatus[prob] == "inf" ) {
         if( !feasible ) {
            if( timeout ) {
               status = "timeout";
               timeouttime += tottime;
               timeouts++;
            }
            else {
               status = "ok";
               pass++;
            }
         }
         else {
            status = "fail";
            failtime += tottime;
            fail++;
         }
      }
      else if( solstatus[prob] == "feas" ) {
         if( feasible ) {
            if( timeout ) {
               status = "timeout";
               timeouttime += tottime;
               timeouts++;
            }
            else {
               status = "ok";
               pass++;
            }
         }
         else {
            status = "fail";
            failtime += tottime;
            fail++;
         }
      }
      else {
         reltol = 1e-5 * max(abs(pb),1.0);
         abstol = 1e-4;

         if( abs(pb - db) < max(abstol,reltol) ) {
            status = "solved not verified";
            pass++;
         }
         else {
            if( timeout || gapreached || sollimitreached ) {
               if( timeout )
                  status = "timeout";
               else if( gapreached )
                  status = "gaplimit";
               else if( sollimitreached )
                  status = "sollimit";
               timeouttime += tottime;
               timeouts++;
            }
            else
               status = "unknown";
         }
      }

      if( writesolufile ) {
         if( pb == +infty && db == +infty )
            printf("=inf= %-18s\n",prob)>NEWSOLUFILE;
         else if( pb == db )
            printf("=opt= %-18s %16.9g\n",prob,pb)>NEWSOLUFILE;
         else if( pb < +infty )
            printf("=best= %-18s %16.9g\n",prob,pb)>NEWSOLUFILE;
         else
            printf("=unkn= %-18s\n",prob)>NEWSOLUFILE;
         #=feas= cannot happen since the problem is reported with an objective value
      }

      #write output to both the tex file and the console depending on whether printsoltimes is activated or not
      if( !onlypresolvereductions || origcons > cons || origvars > vars ) {
         printf("%-19s & %6d & %6d & %16.9g & %16.9g & %6s &%s%8d &%s%7.1f",
                pprob, cons, vars, db, pb, gapstr, markersym, bbnodes, markersym, tottime)  >TEXFILE;
         if( printsoltimes )
            printf(" & %7.1f & %7.1f", timetofirst, timetobest) > TEXFILE;
         printf("\\\\\n") > TEXFILE;

         printf("%-19s %-5s %7d %7d %7d %7d %16.9g %16.9g %6s %8d %7d %7.1f ",
                shortprob, probtype, origcons, origvars, cons, vars, db, pb, gapstr, simpiters, bbnodes, tottime);
         if( printsoltimes )
            printf(" %9.1f %9.1f ", timetofirst, timetobest);
         printf("%s\n", status);
      }

      #PAVER output: see http://www.gamsworld.org/performance/paver/pprocess_submit.htm
      if( solstatus[prob] == "opt" || solstatus[prob] == "feas" )
         modelstat = 1;
      else if( solstatus[prob] == "inf" )
         modelstat = 1;
      else if( solstatus[prob] == "best" )
         modelstat = 8;
      else
         modelstat = 1;
      if( status == "ok" || status == "unknown" )
         solverstat = 1;
      else if( status == "timeout" )
         solverstat = 3;
      else
         solverstat = 10;
      pavprob = prob;
      if( length(pavprob) > 25 )
         pavprob = substr(pavprob, length(pavprob)-24,25);
      printf("%s,MIP,SCIP_%s,0,%d,%d,%g,%g\n", pavprob, settings, modelstat, solverstat, pb, tottime+pavshift) > PAVFILE;
   }
}
END {
   shiftednodegeom -= nodegeomshift;
   shiftedsblpgeom -= sblpgeomshift;
   shiftedtimegeom -= timegeomshift;
   shiftedconftimegeom -= timegeomshift;
   shiftedoverheadtimegeom -= timegeomshift;
   shiftedbasictimegeom -= timegeomshift;
   shiftedtimetofirstgeom -= timegeomshift;
   shiftedtimetobestgeom -= timegeomshift;

   printf("\\midrule\n")                                                 >TEXFILE;
   printf("%-14s (%2d) &        &        &                &                &        & %9d & %8.1f",
          "Total", nprobs, sbab, stottime) >TEXFILE;
   if( printsoltimes )
      printf(" & %8.1f & %8.1f", stimetofirst, stimetobest) > TEXFILE;
   printf("\\\\\n") > TEXFILE;
   printf("%-14s      &        &        &                &                &        & %9d & %8.1f",
          "Geom. Mean", nodegeom, timegeom) >TEXFILE;
   if( printsoltimes )
      printf(" & %8.1f & %8.1f", timetofirstgeom, timetobestgeom) > TEXFILE;
   printf("\\\\\n") > TEXFILE;
   printf("%-14s      &        &        &                &                &        & %9d & %8.1f ",
          "Shifted Geom.", shiftednodegeom, shiftedtimegeom) >TEXFILE;
   if( printsoltimes )
      printf(" & %8.1f & %8.1f", shiftedtimetofirstgeom, shiftedtimetobestgeom) > TEXFILE;
   printf("\\\\\n") > TEXFILE;

   printf(tablehead3);
   printf("\n");

   tablebottom1 = "------------------------------[Nodes]---------------[Time]------";
   tablebottom2 = "  Cnt  Pass  Time  Fail  total(k)     geom.     total     geom.";
   tablebottom3 = "----------------------------------------------------------------";
   
   if( printsoltimes ) {
      tablebottom1 = tablebottom1"--------[ToFirst]-----------[ToLast]-----";
      tablebottom2 = tablebottom2"     total     geom.     total     geom.";
      tablebottom3 = tablebottom3"-----------------------------------------";
   }
   
   tablebottom1 = tablebottom1"\n";
   tablebottom2 = tablebottom2"\n";
   tablebottom3 = tablebottom3"\n";
   
   printf(tablebottom1);
   printf(tablebottom2);
   printf(tablebottom3);
   
   printf("%5d %5d %5d %5d %9d %9.1f %9.1f %9.1f ",
          nprobs, pass, timeouts, fail, sbab / 1000, nodegeom, stottime, timegeom);
   if( printsoltimes )
      printf("%9.1f %9.1f %9.1f %9.1f", stimetofirst, timetofirstgeom, stimetobest, timetobestgeom);
   
   printf("\n");
   printf(" shifted geom. [%5d/%5.1f]      %9.1f           %9.1f ",
          nodegeomshift, timegeomshift, shiftednodegeom, shiftedtimegeom);
   if( printsoltimes )
      printf("          %9.1f           %9.1f ", shiftedtimetofirstgeom, shiftedtimetobestgeom);
   printf("\n");
   printf(tablebottom3);

   printf("\\noalign{\\vspace{6pt}}\n")                                  >TEXFILE;
   printf("\\end{supertabular*}\n")                                      >TEXFILE;
   printf("\\end{center}\n")                                             >TEXFILE;
   printf("\\end{document}\n")                                           >TEXFILE;

   printf("@02 timelimit: %g\n", timelimit);
   printf("@01 SCIP(%s)%s(%s):%s\n", scipversion, lpsname, lpsversion, settings);
}
