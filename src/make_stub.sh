#!/bin/sh

rm -f libstub-*.so
myid=0

stub=$1
shift

#line="ld -L. -shared -o $stub"
line="ld -L. -dynamic -o $stub"

while [ $# -gt 0 ]
do
    myid=`expr $myid + 1`
    lib=stub-$myid
    #ld -soname $1 -shared -o lib$lib.so -lc
    ld -dynamic -o lib$lib.dylib -lc
    #remove="$remove lib$lib.so"
    remove="$remove lib$lib.dylib"
    line="$line -l$lib"
    shift
done

$line

rm $remove
