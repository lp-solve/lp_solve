# $Id: queens4.zpl,v 1.1 2004/12/16 20:22:33 bzfkocht Exp $
#
# This is a formulation of the n queens problem using binary variables.
# variables. Since the number of queens is maximized, the size of the
# board can be set arbitrarily.
#
param columns := 8;

set I   := { 1 .. columns };
set IxI := I * I;

set TABU[<i,j> in IxI] := { <m,n> in IxI with 
   (m != i or n != j) and (m == i or n == j or abs(m - i) == abs(n - j)) };

var x[IxI] binary;

maximize queens: sum <i,j> in IxI : x[i,j];

subto c1: forall <i,j> in IxI do
   forall <m,n> in TABU[i,j] do x[i,j] + x[m,n] <= 1;

   
