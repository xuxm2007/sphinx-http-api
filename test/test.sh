#!/bin/sh
HOST="http://localhost:2046"
INDEX="index=mysql"
SEARCH="$HOST/search?$INDEX"
Q="&q=%E5%B0%91%E7%94%B7"

MH="%3A" #:
DH="%2C" #,
TH="%21" #!
KH_Z="%28" #(
KH_Y="%29" #)
ZKH_Z="%5B" #[
ZKH_Y="%5D" #]

#return `echo -n "$1" | od -An -tx1 | tr ' ' %`

#CURL="curl -v -g -s"
CURL="curl -s -o /dev/null"

info()
{
  #info
  $CURL "$HOST"
  $CURL "$HOST/"
  #status
  $CURL "$HOST/status"
  #statistic
  $CURL "$HOST/statistic"
  $CURL "$HOST/debug"
}
#info;

clean()
{
  M=""; R=""; S=""; ST=""; RS=""; F=""; ID=""; FW=""; G=""; GD=""; SE=""; QT="";
  M="&matchmode=SPH_MATCH_EXTENDED2"
  R="&rankingmode=SPH_RANK_PROXIMITY_BM25"
  S="&sortmode=SPH_SORT_RELEVANCE"
}

search()
{
    url="${SEARCH}${Q}${M}${R}${S}${ST}${RS}${F}${ID}${FW}${G}${GD}${SE}${QT}";
    echo $url;
    $CURL $url;
}

#search -- default
clean
M=""
R=""
S=""
search

#search -- matchmode
search_1()
{
  MMS={SPH_MATCH_ALL SPH_MATCH_ANY SPH_MATCH_PHRASE SPH_MATCH_FULLSCAN SPH_MATCH_EXTENDED2}
  clean
  M=""
  search
  for var in $MMS; do
    M="&matchmode=${var}";
    search
  done
}
#search -- rankingmode
RKS={SPH_RANK_PROXIMITY_BM25 SPH_RANK_BM25 SPH_RANK_NONE SPH_RANK_WORDCOUNT SPH_RANK_PROXIMITY SPH_RANK_FIELDMASK}
clean
search
for var in $RMS; do
  R="&rankingmode=$var"
  search
done

#search -- sortmode
SMS={SPH_SORT_RELEVANCE SPH_SORT_ATTR_DESC${MH}group_id
SPH_SORT_ATTR_ASC${MH}group_id
SPH_SORT_TIME_SEGMENTS${MH}date SPH_SORT_EXTENDED
"SPH_SORT_EXPR:(attrName+attrName2)/2 desc"}
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
QT="&maxquerytime=1000"
search

#%E5%85%AC%E5%BC%80 公开
#%E6%A0%A1%E9%95%BF 校长
#%E9%93%B6%E5%85%83 银元
#%E5%B0%91%E7%94%B7 少男

