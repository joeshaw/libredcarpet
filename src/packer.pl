#!/usr/bin/perl

$name = $ARGV[1];
$name =~ s/[{}]//g;
$name =~ s/[-\.]/_/g;

open FOO, "$ARGV[0]";

$total = 0;

print "static const char ", $name, "[] = { ";

while ($len = read (FOO, $buf, 100)) {
    $total += $len;
    foreach $char (split //, $buf) {
        printf ("0x%X, ", ord ($char));
    }
}

print "};\n";
print "static unsigned int ", $name, "_len = ", $total, ";\n";
