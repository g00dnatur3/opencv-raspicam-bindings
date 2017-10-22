#!/bin/bash --debugger
set -e

[ -z $BASH ] || shopt -s expand_aliases
alias BEGINCOMMENT="if [ ]; then"
alias ENDCOMMENT="fi"

cd $HOME

[ ! -d raspicam ] && wget -O raspicam.zip https://downloads.sourceforge.net/project/raspicam/raspicam-0.1.6.zip && unzip raspicam.zip -d . && rm -rf raspicam.zip

cd raspicam-0.1.6

[ ! -d build ] && mkdir build
cd build
cmake ..

make
sudo make install
sudo ldconfig