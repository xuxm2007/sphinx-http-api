#!/bin/sh
#./buildconf.sh
./configure --prefix=$HOME/local/coreseek --with-mmseg \
  --with-mmseg-includes=$HOME/local/mmseg/include/mmseg \
  --with-mmseg-libs=$HOME/local/mmseg/lib --enable-id64

make install
