#!/bin/sh
DIR=$(cd "$(dirname "$0")"; pwd)

MYSQL=/home/zhaigy/local/mysql/bin/mysql
USER=root
PASSWORD=root

DATABASE2=test
USER2=test
PASSWORD2=test

CORESEEK=/home/zhaigy/local/coreseek
INDEXER=$CORESEEK/bin/indexer
SEARCHER=$CORESEEK/bin/searchd

#$SEARCHER --config $DIR/csft_mysql.conf --index mysql
