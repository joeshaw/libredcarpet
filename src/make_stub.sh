#!/bin/sh

rm -f libstub-*.so
myid=0

stub=$1
shift

line="ld -L. -shared -o $stub"

while [ $# -gt 0 ]
do
    myid=`expr $myid + 1`
    lib=stub-$myid
    ld -soname $1 -shared -o lib$lib.so -lc
    remove="$remove lib$lib.so"
    line="$line -l$lib"
    shift
done

$line

rm $remove
