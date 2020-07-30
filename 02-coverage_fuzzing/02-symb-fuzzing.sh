#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-symb
cd work-symb
../../aflplusplus/afl-fuzz -i ../inputs/loadpng -o ../fuzz-sym/ -- ../bin/loadpng_symb_inst @@
