#!/bin/sh

stub=$1
shift

line="ld -T ld_script -shared -o $stub"

while [ $# -gt 1 ]
do
    ld -soname $1 -shared -o lib$2.so -lc
    remove="$remove lib$2.so"
    line="$line -l$2"
    shift
    shift
done

cat << 'EOF' >munge_ld_script
$in_script = 0;
while (<>) {
  exit if (/^====/ && $in_script);
  $_ = "SEARCH_DIR(.);\n" if (/^SEARCH_DIR/);
  print if ($in_script);
  $in_script = 1 if (/^====/);
}
EOF
ld --verbose | perl munge_ld_script > ld_script
remove="$remove ld_script munge_ld_script"

$line

for file in $remove
do
    rm $file
done
