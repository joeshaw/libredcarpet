#!/bin/sh

stub=$1
shift

line="ld -L. -shared -o $stub"

while [ $# -gt 0 ]
do
    lib=`mktemp -u XXXXXXXX`
    ld -soname $1 -shared -o lib$lib.so -lc
    remove="$remove lib$lib.so"
    line="$line -l$lib"
    shift
done

$line

rm $remove
