#!/usr/bin/env bash
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

TSTNAME=$1
GUROBIBIN=$2
SETNAME=$3
BINNAME=$GUROBIBIN.$4
TIMELIMIT=$5
NODELIMIT=$6
MEMLIMIT=$7
THREADS=$8
FEASTOL=$9
MIPGAP=${10}
CONTINUE=${11}

GRBNODEFILEDIR=gurobi_nodefiledir

if test ! -e results
then
    mkdir results
fi
if test ! -e settings
then
    mkdir settings
fi

OUTFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.out
ERRFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.err
RESFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.res
TEXFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.tex
TMPFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.tmp
SETFILE=results/check.$TSTNAME.$BINNAME.$SETNAME.prm

SETTINGS=settings/$SETNAME.grbset

if test "$CONTINUE" = "true"
then
    MVORCP=cp
else
    MVORCP=mv
fi

DATEINT=`date +"%s"`
if test -e $OUTFILE
then
    $MVORCP $OUTFILE $OUTFILE.old-$DATEINT
fi
if test -e $ERRFILE
then
    $MVORCP $ERRFILE $ERRFILE.old-$DATEINT
fi

if test "$CONTINUE" = "true"
then
    LASTPROB=`./getlastprob.awk $OUTFILE`
    echo Continuing benchmark. Last solved instance: $LASTPROB
    echo "" >> $OUTFILE
    echo "----- Continuing from here. Last solved: $LASTPROB -----" >> $OUTFILE
    echo "" >> $OUTFILE
else
    LASTPROB=""
fi

uname -a >>$OUTFILE
uname -a >>$ERRFILE
date >>$OUTFILE
date >>$ERRFILE

# we add 10% to the hard time limit and additional 10 seconds in case of small time limits
HARDTIMELIMIT=`expr \`expr $TIMELIMIT + 10\` + \`expr $TIMELIMIT / 10\``

# since bash counts cpu time we need the time limit for each thread
HARDTIMELIMIT=`expr $HARDTIMELIMIT \* $THREADS`

# we add 10% to the hard memory limit and additional 100mb to the hard memory limit
HARDMEMLIMIT=`expr \`expr $MEMLIMIT + 100\` + \`expr $MEMLIMIT / 10\``
HARDMEMLIMIT=`expr $HARDMEMLIMIT \* 1024`

# convert MEMLIMIT to GB for gurobi paramter (uses bc for fractional value)
GRBMEMLIMIT=`echo "$MEMLIMIT / 1024" | bc -l`
rm -f $TMPFILE

echo "hard time limit: $HARDTIMELIMIT s" >>$OUTFILE
echo "hard mem limit: $HARDMEMLIMIT k" >>$OUTFILE

for i in `cat $TSTNAME.test`
do
    if test "$LASTPROB" = ""
    then
        LASTPROB=""
        if test -f $i
        then
            rm -f $SETFILE
            echo @01 $i ===========
            echo @01 $i ===========                 >> $ERRFILE
            echo "from gurobipy import *" > $TMPFILE
            echo "print 'Gurobi Interactive Shell, Version %s' % '.'.join(str(x) for x in gurobi.version())" >> $TMPFILE
#equivalent formulation from gurobi.py
#echo "version = str(gurobi.version()[0]) + '.' + str(gurobi.version()[1]) + '.' + str(gurobi.version()[2])" >> $TMPFILE
#echo "print('\nGurobi Interactive Shell, Version ' + version)" >> $TMPFILE
#echo "print('Copyright (c) 2009, Gurobi Optimization, Inc.')" >> $TMPFILE
            if test $SETNAME != "default"
            then
                echo "readParams("$SETTINGS")"        >> $TMPFILE
            fi
            if test $FEASTOL != "default"
            then
                echo "setParam(\"FeasibilityTol\",$FEASTOL)" >> $TMPFILE
                echo "setParam(\"IntFeasTol\",$FEASTOL)"     >> $TMPFILE
            fi
            echo "setParam(\"TimeLimit\",$TIMELIMIT)" >> $TMPFILE
            echo "setParam(\"DisplayInterval\",100)"  >> $TMPFILE
            if test $MIPGAP != "default"
            then
                echo "setParam(\"MIPGap\",$MIPGAP)"   >> $TMPFILE
            fi
            echo "setParam(\"NodeLimit\",$NODELIMIT)" >> $TMPFILE
            echo "setParam(\"NodefileStart\",$GRBMEMLIMIT)" >> $TMPFILE
            rm -fr $GRBNODEFILEDIR
            mkdir $GRBNODEFILEDIR
            echo "setParam(\"NodefileDir\",\"$GRBNODEFILEDIR\")" >> $TMPFILE
            echo "setParam(\"Threads\",$THREADS)"     >> $TMPFILE
            echo "writeParams(\"$SETFILE\")"          >> $TMPFILE
            echo "problem=read(\"$i\")"               >> $TMPFILE
            echo "problem.printStats()"               >> $TMPFILE
            echo "problem.optimize()"                 >> $TMPFILE
            echo "quit()"                             >> $TMPFILE
            echo -----------------------------
            date
            date >>$ERRFILE
            echo -----------------------------
            date +"@03 %s"
            bash -c "ulimit -t $HARDTIMELIMIT; ulimit -v $HARDMEMLIMIT; ulimit -f 1000000; $GUROBIBIN < $TMPFILE" 2>>$ERRFILE
            date +"@04 %s"
            echo -----------------------------
            date
            date >>$ERRFILE
            echo -----------------------------
            echo =ready=
        else
            echo @02 FILE NOT FOUND: $i ===========
            echo @02 FILE NOT FOUND: $i =========== >>$ERRFILE
        fi
    else
        echo skipping $i
        if test "$LASTPROB" = "$i"
        then
            LASTPROB=""
        fi
    fi
done | tee -a $OUTFILE

rm -f $TMPFILE
rm -fr $GRBNODEFILEDIR

date >>$OUTFILE
date >>$ERRFILE

if test -f $TSTNAME.solu
then
    awk -f check_gurobi.awk -vTEXFILE=$TEXFILE $TSTNAME.solu $OUTFILE | tee $RESFILE
else
    awk -f check_gurobi.awk -vTEXFILE=$TEXFILE $OUTFILE | tee $RESFILE
fi
