#!/usr/bin/perl

$name = $ARGV[1];
$name =~ s/[{}]//g;
$name =~ s/[-\.]/_/g;

open FOO, "$ARGV[0]";
binmode FOO;

$total = 0;

print "static const char ", $name, "[] = { ";

while ($len = read (FOO, $buf, 100)) {
    if($total != 0){
	printf(",");
    }
    $total += $len;
    @list = (split //, $buf);
    for($i = 0; $i < @list; ++$i) {
        printf ("0x%X", ord ($list[$i]));
	if($i+1 != @list) {
		printf(",");
	}
	printf(" ");
    }
}

print "};\n";
print "static unsigned int ", $name, "_len = ", $total, ";\n";
