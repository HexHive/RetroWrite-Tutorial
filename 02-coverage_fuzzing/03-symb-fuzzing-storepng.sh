#!/bin/bash

# be noisy
set -ex

cd ../playground/

mkdir -p work-symb-storepng
cd work-symb-storepng
../../aflplusplus/afl-fuzz -i ../inputs/storepng -o ../fuzz-sym/ -- ../bin/storepng_symb_inst @@
