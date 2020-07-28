#!/bin/bash

# Display all commands on the command line so users can see 
# what we're doing and why
set -ex

# variables (how to find things) go here:
AFLPP=$(pwd)/aflplusplus

# Tune system to work with AFL++
sudo ./setup_root.sh || exit 1

# build AFL++:
echo "[*] Cloning AFL++ into $AFLPP"
if !test -d $AFLPP; then
    git clone https://github.com/AFLplusplus/AFLplusplus $AFLPP
fi

pushd $AFLPP
git checkout "2.66c"

echo "[*] Building AFL++"

make || exit 1
echo "[*] Building AFL++ qemu_mode support"
pushd $AFLPP/qemu_mode
./build_qemu_support.sh || exit 1
popd && popd


