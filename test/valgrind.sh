#!/bin/sh
DIR=$(cd $(dirname $0);pwd)
WORK_HOME=$DIR/..
cd $WORK_HOME
valgrind --leak-check=full --show-reachable=yes --track-fds=yes\
  --trace-children=yes\
  --log-file-exactly=./test/memcheck.log\
  ./sphinx_http_api &
cpid=$!

sleep 8;
cd $DIR

for((i=0;i<10;++i)); do
  sh ./test.sh
done

sleep 1
kill $cpid
sleep 1


#--tool=helgrind 
#--tool=memcheck
