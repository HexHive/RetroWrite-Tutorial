#!/bin/bash

# be noisy
set -ex

# locate this script:
SCRIPT=$(readlink -f $0)
SCRIPTDIR=$(dirname $SCRIPT)

# cd into the right place
cd "$SCRIPTDIR/../playground"
# make directory
mkdir -p work-native-storepng
mkdir -p work-native-loadpng
