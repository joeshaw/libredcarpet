#!/usr/bin/perl

open FOO, "$ARGV[0]";

$total = 0;

print "static const char ", $ARGV[1], "[] = { ";

while ($len = read (FOO, $buf, 100)) {
    $total += $len;
    foreach $char (split //, $buf) {
        printf ("0x%X, ", ord ($char));
    }
}

print "};\n";
print "static unsigned int ", $ARGV[1], "_len = ", $total, ";\n";
