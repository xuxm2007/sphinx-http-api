#!/bin/bash
DIR=$(cd $(dirname $0);pwd)
WORK_HOME=$DIR/..
cd $WORK_HOME
valgrind --leak-check=full --show-reachable=yes --track-fds=yes --trace-children=yes --log-file=./test/memcheck.log ./sphinx_http_api &
cpid=$!

sleep 3;
cd $DIR

for ((i=0; i<2; ++i)) ; do
  sh ./test.sh
  sleep 1
  sh ./test_fail.sh
done

sleep 1
kill $cpid
sleep 1

#--tool=helgrind 
#--tool=memcheck
