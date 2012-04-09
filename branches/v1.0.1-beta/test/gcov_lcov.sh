#!/bin/sh
DIR=$(cd $(dirname $0);pwd)
WORK_HOME=$DIR/..

cd $WORK_HOME

make

./sphinx_http_api &

cpid=$!
sleep 5

cd $DIR
sh ./test.sh
sleep 3
sh ./test_fail.sh
sleep 3
kill $cpid
sleep 3

T=cov.test.info
rm -rf $T
lcov --capture --directory $WORK_HOME --base-directory $WORK_HOME --output-file $T 
rm -rf ./output
genhtml $T --output-directory ./output --title "test" --show-details --legend
rm -rf $T
