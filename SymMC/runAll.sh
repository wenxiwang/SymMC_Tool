#!/bin/bash

currentdir="$(dirname "$(realpath $0)")"
minisat=$currentdir/SymMC/cmake-build-release/minisat
cnfDir=$currentdir/InputFiles/cnfs

for f in $(find $cnfDir -name '*.cnf'); 
do 
  cnfname=$f
  tmp2=${cnfname/cnfs/SymInfo}
  permname=${tmp2/.cnf/".perm"}
  printf '%s' "$cnfname"

  "$minisat" "$cnfname" "$permname" 

done
