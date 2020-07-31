#!/bin/bash

# Display all commands on the command line so users can see 
# what we're doing and why
set -ex

# variables (how to find things) go here:
AFLPP=$(pwd)/aflplusplus
DISTRO=`awk -F= '/^NAME/{print $2}' /etc/os-release | tr -d '"'`

if [[ "Ubuntu" == "$DISTRO" ]]; then
# package install
sudo apt-get install -y make autoconf automake libtool shtool wget curl \
                       xz-utils gcc g++ cmake \
                       ninja-build zlib1g make python \
                       build-essential git ca-certificates \
                       tar gzip vim libelf-dev libelf1 libiberty-dev \
                       libboost-all-dev python3-pip python3-venv \
                       libpcap-dev libbz2-dev liblzo2-dev liblzma-dev liblz4-dev libz-dev \
                       libxml2-dev libssl-dev libacl1-dev libattr1-dev zip \
                       unzip libtool-bin bison flex libpixman-1-dev
fi

# trigger download of retrowrite and other submodules
git submodule update --init --recursive

# Tune system to work with AFL++
echo 'core' | sudo tee /proc/sys/kernel/core_pattern
if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
pushd /sys/devices/system/cpu 
echo performance | sudo tee cpu*/cpufreq/scaling_governor
popd
fi


# build AFL++:
echo "[*] Cloning AFL++ into $AFLPP"
if [[ ! -d $AFLPP ]]; then
    git clone https://github.com/AFLplusplus/AFLplusplus $AFLPP
fi

pushd $AFLPP
git checkout "2.66c"

echo "[*] Building AFL++"
export AFL_USE_ASAN=1 
make ASAN_BUILD=1 || exit 1
echo "[*] Building AFL++ qemu_mode support"
pushd $AFLPP/qemu_mode
./build_qemu_support.sh || exit 1
popd && popd

echo "[*] Building Retrowrite using Retrowrite's helper script"
pushd retrowrite
./setup.sh
popd
