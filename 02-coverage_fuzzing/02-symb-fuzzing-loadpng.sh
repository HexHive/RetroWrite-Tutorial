#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-symb-loadpng
cd work-symb-loadpng
../../aflplusplus/afl-fuzz -i ../inputs/loadpng -o ../fuzz-sym/ -- ../bin/loadpng_symb_inst @@
