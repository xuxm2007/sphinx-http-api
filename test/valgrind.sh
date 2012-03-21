#!/bin/sh
DIR=$(cd $(dirname $0);pwd)
WORK_HOME=$DIR/..
cd $WORK_HOME
valgrind --leak-check=full --show-reachable=yes --track-fds=yes\
  --trace-children=yes ./sphinx_http_api

#--tool=helgrind 
#--tool=memcheck
