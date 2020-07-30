#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-asan
cd work-asan
../../aflplusplus/afl-fuzz -i ../inputs/loadpng -o ../fuzz-asan/ -- ../bin/loadpng_asan_inst @@
