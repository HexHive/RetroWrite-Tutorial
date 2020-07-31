#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-asan-loadpng
cd work-asan-loadpng
../../aflplusplus/afl-fuzz -i ../inputs/loadpng -o ../03-fuzz-asan-loadpng/ -- ../bin/loadpng_asan_inst @@
