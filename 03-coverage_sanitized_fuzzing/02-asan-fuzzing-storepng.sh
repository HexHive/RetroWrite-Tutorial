#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-asan
cd work-asan
../../aflplusplus/afl-fuzz -i ../inputs/storepng -o ../fuzz-asan2/ -- ../bin/storepng_asan_inst @@
