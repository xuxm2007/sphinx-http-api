#!/bin/sh

. ./head.sh

info()
{
  #info
  $CURL "$HOST/1"
  $CURL "$HOST/i.h"
  $CURL "$HOST/1/"
  $CURL "$HOST/1/i.h"
  $CURL "$HOST/status//"
  $CURL "$HOST//s"
  $CURL "$HOST//statistic"
  $CURL "$HOST/././debug/../"
}
info;

#search -- matchmode
MMS={SPH_MATCH_ALL_ERR "%20SPH_MATCH_EXTENDED2%20" "+SPH_MATCH_EXTENDED2+" "=SPH_MATCH_EXTENDED2"}
clean
M=""
search
for var in $MMS; do
  M="&matchmode=${var}";
  search
done
M="&matchmode=SPH_MATCH_ALL"
M=$M$M
search
M="&amp;matchmode=SPH_MATCH_ALL"
search
M="%26matchmode=SPH_MATCH_ALL"
#%26 = &
search

#search -- rankingmode
RKS={SPH_RANK_PROXIMITY_BM25_}
clean
for var in $RMS; do
  R="&rankingmode=$var"
  search
done

#search -- sortmode
SMS={SPH_SORT_ATTR_DESC${MH}group_id SPH_SORT_TIME_SEGMENTS${MH}date "SPH_SORT_EXPR:(attrName+attrName2)/2 desc"}
clean
for var in $SMS; do
  S="&sortmode=$var"
  search
done

#search -- groupby
clean
G="&groupby=SPH_GROUPBY_DAY${MH}group_id${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_WEEK${MH}date_added${MH}@group+desca"
search

#groupdistinct

#search -- idrange
clean
ID="&idrange=1,10000"
search
ID="&idrange=1,2,10000"
search

#search -- filter
clean
F="&filter=group_id:1,2,3,4,5,6,7,8,9,10"
search
F="&filter=group_id${MH}${ZKH_Z}1TO10${ZKH_Y}"
search
F="&filter=group_id${MH}${ZKH_Z}1+TO+10+TO10${ZKH_Y}"
search
F="&filter=${TH}${TH}group_id${MH}1${DH}2${DH}3${DH}4${DH}5${DH}6${DH}7${DH}8${DH}9${DH}10"
search
F="&filter=group_id${MH}${ZKH_Z}${ZKH_Z}1+TO+10${KH_Y}"
search
F="&filter=group_id${MH}${ZKH_Z}1.1.1+TO+-010${KH_Y}"
search

#search -- fieldweights
clean
FW="&fieldweights=title${MH}${MH}2${DH}content${MH}1${MH}2"
search
FW="&fieldweights=title${MH}-2${DH}content${MH}12"
search
FW="&fieldweights=title${MH}-2${DH}content2"
search
FW="&fieldweights=title${MH}-2${DH}${DH}content2"
search
FW="&fieldweights=title${MH}0.2${DH}content2"
search
FW="&fieldweights=title${MH}2.2..2${DH}content${MH}1"
search
FW="&fieldweights=title${MH}f.2${DH}content2"
search
FW="&fieldweights=${MH}title${MH}2${DH}content${MH}1"
search
FW="&fieldweights=title${MH}2${DH}content${MH}9999999999999999999999999999"
search

#search -- limit
clean
ST="&start=-2"
RS="&rows=8"
search
ST="&start=20"
search
RS="&rows=-1"
search

#search -- maxquerytime
clean
QT="&maxquerytime=10000000000000000000000000000"
search

clean
QT="&maxquerytime=10000000000000000000000000000"
for((i=0;i<20;++i)); do
  QT="${QT}00000000000000000000000000000000000000000000000000000000000000000000"
done
search
for((i=0;i<100;++i)); do
  QT="${QT}00000000000000000000000000000000000000000000000000000000000000000000"
done
search
