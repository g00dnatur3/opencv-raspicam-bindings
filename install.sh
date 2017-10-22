#!/bin/bash

#sudo apt-get install -y --force-yes --no-install-recommends cmake libgtk2.0-dev

#--------------------------------------------------------------------
function tst {
    echo "===> Executing: $*"
    if ! $*; then
        echo "Exiting script due to error from: $*"
        exit 1
    fi
}
#--------------------------------------------------------------------

if hash cmake-js 2>/dev/null; then
	# do nothing because its installed
	echo "cmake-js is already installed"
else
	# install cmake-js
	tst sudo npm install -g cmake-js@^3.4.1
fi

tst cd lib/opencv
tst npm install
tst cd ../..
