#!/bin/sh

. ./head.sh
#search -- select 
clean
SE="&select=id${DH}group_id"
search

info()
{
  #info
  $CURL "$HOST"
  $CURL "$HOST/"
  $CURL "$HOST/info"
  #status
  $CURL "$HOST/status"
  #statistic
  $CURL "$HOST/statistic"
  $CURL "$HOST/debug"
}
info;

#search -- default
clean
M=""
R=""
S=""
search

#search -- matchmode
search_1()
{
  MMS=("SPH_MATCH_ALL" "SPH_MATCH_ANY" "SPH_MATCH_PHRASE" "SPH_MATCH_FULLSCAN"
    "SPH_MATCH_EXTENDED2")
  clean
  M=""
  search
  for var in $MMS; do
    M="&matchmode=${var}";
    search
  done
}
search_1;

#search -- rankingmode
RKS=(SPH_RANK_PROXIMITY_BM25 SPH_RANK_BM25 SPH_RANK_NONE SPH_RANK_WORDCOUNT
SPH_RANK_PROXIMITY SPH_RANK_FIELDMASK)
clean
search
for var in $RMS; do
  R="&rankingmode=$var"
  search
done

#search -- sortmode
SMS=(SPH_SORT_RELEVANCE SPH_SORT_ATTR_DESC${MH}group_id
SPH_SORT_ATTR_ASC${MH}group_id
SPH_SORT_TIME_SEGMENTS${MH}date SPH_SORT_EXTENDED
"SPH_SORT_EXPR${MH}${KH_L}attrName+attrName2${KH_R}${FXG}2+desc")
clean
for var in $SMS; do
  S="&sortmode=$var"
  search
done

#search -- groupby
clean
G="&groupby=SPH_GROUPBY_ATTR${MH}group_id${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_DAY${MH}date_added${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_WEEK${MH}date_added${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_MONTH${MH}date_added${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_YEAR${MH}date_added${MH}@group+desc"
search
G="&groupby=SPH_GROUPBY_YEAR${MH}date_added${MH}@group+desc"
GD="&groupdistinct=group_id"
search

#groupdistinct

#search -- idrange
clean
ID="&idrange=1,10000"
search

#search -- filter
clean
F="&filter=group_id${MH}1${DH}2${DH}3${DH}4${DH}5${DH}6${DH}7${DH}8${DH}9${DH}10"
search
F="&filter=group_id${MH}${ZKH_Z}1+TO+10${ZKH_Y}"
search
F="&filter=${TH}group_id${MH}1${DH}2${DH}3${DH}4${DH}5${DH}6${DH}7${DH}8${DH}9${DH}10"
search
F="&filter=${TH}group_id${MH}${ZKH_Z}1+TO+10${ZKH_Y}"
search
F="&filter=group_id${MH}${KH_Z}1+TO+10${KH_Y}"
search
F="&filter=group_id${MH}${ZKH_Z}1+TO+10${KH_Y}"
search
F="&filter=${TH}group_id${MH}${ZKH_Z}1+TO+10${KH_Y}"
search
F="&filter=group_id${MH}${ZKH_Z}1.1+TO+10${KH_Y}"
search

#search -- fieldweights
clean
FW="&fieldweights=title${MH}2${DH}content${MH}1"
search

#search -- limit
clean
ST="&start=2"
RS="&rows=8"
search

#search -- maxquerytime
clean
QT="&maxquerytime=1000&connecttimeout=1000"
search

#search -- select 
clean
SE="&select=id${DH}group_id"
search
