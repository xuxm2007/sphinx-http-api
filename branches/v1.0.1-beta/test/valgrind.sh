#!/bin/bash
DIR=$(cd $(dirname $0);pwd)
WORK_HOME=$DIR/..
cd $WORK_HOME
make;
rm -f ./test/memcheck.log*;
valgrind --leak-check=full --show-reachable=yes --track-fds=yes \
  --trace-children=yes --log-file=./test/memcheck.log -v \
  ./sphinx_http_api &
cpid=$!

sleep 2 
cd $DIR

for ((i=0; i<1; ++i)) ; do
  sh ./test.sh
  sh ./test_fail.sh
done

sleep 1
echo "kill $cpid"
kill $cpid
sleep 1

#--tool=helgrind 
#--tool=memcheck
