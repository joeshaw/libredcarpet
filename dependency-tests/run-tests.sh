#!/bin/sh

cd $1

#TEST_TESTER=1

if [ -n "$COLORTERM" -o $TERM = "xterm" -o $TERM = "rxvt" -o $TERM = "xterm-color" ]; then
	SETCOLOR_SUCCESS="echo -en \\033[1;32m"
	SETCOLOR_FAILURE="echo -en \\033[1;31m"
	SETCOLOR_WARNING="echo -en \\033[1;33m"
	SETCOLOR_NORMAL="echo -en \\033[0;39m"
else
	SETCOLOR_SUCCESS=
	SETCOLOR_FAILURE=
	SETCOLOR_WARNING=
	SETCOLOR_NORMAL=
fi

for i in `ls *-test.xml | sort -t - -k 2 -n` ; do
	base=`echo $i | sed -e s,\.xml$,,`
	if [ ! -r $base.solution ]; then
		echo -n "$base.xml "
		$SETCOLOR_WARNING
		echo -n WARNING
		$SETCOLOR_NORMAL
		echo , missing solution
		warning=$(( $warning + 1 ))
		continue
	fi

	../deptestomatic $base.xml | fgrep '>!>' > $base.mistake
	diff -U 500 $base.solution $base.mistake >$base.diff 2>&1

	if [ $? -eq 0 ] ; then
		echo -n "$base.xml "
		$SETCOLOR_SUCCESS
		echo PASSED
		$SETCOLOR_NORMAL
		rm -f $base.mistake $base.diff
		success=$(( $success + 1 ))
	else
		echo -n "$base.xml "
		$SETCOLOR_FAILURE
		if [ -e core ] ; then
			echo CRASH
			$SETCOLOR_NORMAL
			crash=$(( $crash + 1 ))
		else
			echo -n FAILED
			$SETCOLOR_NORMAL
			echo , check $base.mistake and $base.diff
			failure=$(( $failure + 1 ))
		fi

	fi
done

echo -n "Summary: "

if [ $failure ] ; then
	$SETCOLOR_FAILURE
else
	$SETCOLOR_SUCCESS
fi

if [ $success ] ; then
	if [ $success -eq "1" ] ; then
		echo -n 1 Success
	else
		echo -n $success Successes
	fi
else
	$SETCOLOR_NORMAL
	echo -n No Successes
fi

if [ $failure ] ; then
	$SETCOLOR_FAILURE
else
	$SETCOLOR_NORMAL
fi

echo -n ", "

if [ $warning ] ; then

	if [ ! $failure ] ; then
		$SETCOLOR_WARNING
	fi
	if [ $warning -eq "1" ] ; then
		echo -n "1 Warning"
	else
		echo -n "$warning Warnings"
	fi
fi

if [ $failure ] ; then
	$SETCOLOR_FAILURE
else
	$SETCOLOR_NORMAL
fi

echo -n ", "

if [ $failure ] ; then

	if [ $failure -eq "1" ] ; then
		echo -n 1 Failure
	else
		echo -n $failure Failures
	fi
else
	echo -n No Failures
fi

if [ $crash ] ; then
	$SETCOLOR_FAILURE
else
	$SETCOLOR_NORMAL
fi

echo -n ", "

if [ $crash ] ; then

	if [ $crash -eq "1" ] ; then
		echo -n 1 Crash
	else
		echo -n $crash Crashes
        fi
else
    echo -n No Crashes
fi

$SETCOLOR_NORMAL

echo .
