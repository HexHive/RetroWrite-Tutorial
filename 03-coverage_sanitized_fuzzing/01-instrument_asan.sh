#!/bin/bash

# be noisy
set -ex

cd ../playground/bin
retrowrite loadpng loadpng_asan.s --asan
sed -i 's/asan_init_v4/asan_init/g' loadpng_asan.s
AFL_AS_FORCE_INSTRUMENT=1 $AFL_PATH/afl-gcc loadpng_asan.s -o loadpng_asan_inst -lz -fsanitize=address


retrowrite storepng storepng_asan.s -a
sed -i 's/asan_init_v4/asan_init/g' storepng_asan.s
AFL_AS_FORCE_INSTRUMENT=1 $AFL_PATH/afl-gcc storepng_asan.s -o storepng_asan_inst -lz -fsanitize=address

