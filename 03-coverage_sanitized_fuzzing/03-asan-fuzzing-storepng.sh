#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-asan-storepng
cd work-asan-storepng
../../aflplusplus/afl-fuzz -i ../inputs/storepng -o ../03-fuzz-asan-storepng/ -- ../bin/storepng_asan_inst @@
