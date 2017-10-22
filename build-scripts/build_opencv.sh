#!/bin/bash --debugger
set -e

[ -z $BASH ] || shopt -s expand_aliases
alias BEGINCOMMENT="if [ ]; then"
alias ENDCOMMENT="fi"

cd $HOME
[ ! -d opencv ] && mkdir opencv
cd opencv

if uname -n | grep -q raspberrypi; then
    echo "RPI BUILD!"
    RPI="1"
fi

BEGINCOMMENT

# Update and Upgrade the Pi, otherwise the build may fail due to inconsistencies
sudo apt-get update && sudo apt-get upgrade -y --force-yes

ENDCOMMENT      

# Get the required libraries
sudo apt-get install -y --force-yes --no-install-recommends \
									build-essential cmake git libgtk2.0-dev \
									pkg-config libavcodec-dev libavformat-dev libswscale-dev \
									libjpeg-dev libpng-dev libtiff-dev libjasper-dev

# get src if they are not there yet

[ ! -d opencv-3.2.0 ] && wget https://github.com/opencv/opencv/archive/3.2.0.tar.gz && tar xvf 3.2.0.tar.gz && rm -rf 3.2.0.tar.gz
[ ! -d opencv_contrib-3.2.0 ] && wget https://github.com/opencv/opencv_contrib/archive/3.2.0.tar.gz && tar xvf 3.2.0.tar.gz && rm -rf 3.2.0.tar.gz

# append to basrc if not exists
grep -q -F 'export LD_LIBRARY_PATH=/usr/local/lib/' $HOME/.bashrc || echo 'export LD_LIBRARY_PATH=/usr/local/lib/' >> $HOME/.bashrc

export LD_LIBRARY_PATH=/usr/local/lib/

###  BUILD OPENCV

cd opencv-3.2.0
ls build || mkdir build
cd build

cmake \
	-D OPENCV_EXTRA_MODULES_PATH=$HOME/opencv/opencv_contrib-3.2.0/modules \
	-D CMAKE_BUILD_TYPE=Release \
	-D CMAKE_INSTALL_PREFIX=/usr/local ..
	
make -j4

sudo make install 


