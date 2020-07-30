#!/bin/bash
# be noisy
set -ex

# locate this script:
SCRIPT=$(readlink -f $0)
SCRIPTDIR_THIS=$(dirname $SCRIPT)

# if the user didn't source tools, let's do it for them:
# but if they did, don't do it twice.
if ! command -v afl-fuzz; then
    cd $SCRIPTDIR_THIS/..
    source tools
fi

# cd into the right place
cd "$SCRIPTDIR_THIS/../playground/work-native-loadpng"

# exec afl
afl-fuzz -Q -i ../inputs/loadpng -o ../fuzz-native-loadpng/ -- ../bin/loadpng @@
