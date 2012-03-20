#!/usr/bin/env sh
MYSQL=`which mysql`
if [ -z $MYSQL ]; then
  MYSQL=/home/zhaigy/local/mysql/bin/mysql
fi

N=$1

if [ "$N" = "" ]; then
  N=2
fi

DATABASE2=test
USER2=test
PASSWORD2=test

sqlfile=./test.create.data.mysql.2.txt

create_data() {
  if [ ! -f $sqlfile ]; then
    perl create_data.pl
    sleep 1;
  fi
  $MYSQL -u $USER2 --password=$PASSWORD2 -h localhost $DATABASE2 < $sqlfile
  rm -f $sqlfile
}

for i in {1 2}; do
  create_data
done
