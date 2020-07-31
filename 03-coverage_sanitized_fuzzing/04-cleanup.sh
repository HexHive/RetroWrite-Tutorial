#!/bin/bash

# be noisy:
set -ex

# locate this script:
SCRIPT=$(readlink -f $0)
SCRIPTDIR=$(dirname $SCRIPT)

# cd into the right place
cd $SCRIPTDIR/../playground
# remove work directory
rm -rf work-asan-loadpng
rm -rf work-asan-storepng
