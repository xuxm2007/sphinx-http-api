#!/bin/sh
HOST="http://localhost:2046"
INDEX="index=mysql"
SEARCH="$HOST/search?$INDEX"
Q="&q=%E5%B0%91%E7%94%B7"
#%E5%85%AC%E5%BC%80 公开
#%E6%A0%A1%E9%95%BF 校长
#%E9%93%B6%E5%85%83 银元
#%E5%B0%91%E7%94%B7 少男

MH="%3A" #:
DH="%2C" #,
TH="%21" #!
FXG="%2F" #/
KH_Z="%28" #(
KH_Y="%29" #)
ZKH_Z="%5B" #[
ZKH_Y="%5D" #]

#return `echo -n "$1" | od -An -tx1 | tr ' ' %`

#CURL="curl -v -g -s"
CURL="curl -s -o /dev/null"

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

