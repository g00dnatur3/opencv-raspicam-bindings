#!/bin/bash --debugger
set -e

cd $HOME
[ ! -d nodejs ] && mkdir nodejs
cd nodejs

[ ! -d node-6.10.3 ] && wget https://github.com/nodejs/node/archive/v6.10.3.tar.gz && tar xvf v6.10.3.tar.gz && rm -rf v6.10.3.tar.gz

cd node-6.10.3

./configure

make -j4

sudo make install 