#!/bin/bash

rm -rf 2
rm new_5
rm client_output
mkdir 2
touch 2/1
touch 2/2
touch 2/3
touch 2/4
dd if=/dev/urandom of=2/5 bs=1024 count=1024 &> /dev/null
echo "cd 2
ls .
cp 5
exit exit
" > commands

./run_experiment.sh $1 $2

DIFF=$(diff client_output test2)
DIFF2=$(diff 2/5 new_5)
if [ "$DIFF" == "" -a "$DIFF2" == "" ]
then
    echo "PASS"
else
	echo "FAIL"
fi

