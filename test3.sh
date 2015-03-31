#!/bin/bash

rm -rf 3
rm client_output
mkdir 3
touch 3/1
touch 3/2
touch 3/3
touch 3/4
echo "cd 3
ls .
sn server
ls .
exit exit
" > commands

./run_experiment.sh $1 $2

DIFF=$(diff client_output test3)
DIFF2=$(diff server 3/new_server)
if [ "$DIFF" == "" -a "$DIFF2" == "" ] 
then
    echo "PASS"
else
	echo "FAIL"
fi

