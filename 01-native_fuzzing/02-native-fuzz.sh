#!/bin/bash
# cd into the right place
cd ../playground/work-native
# exec afl
../../aflplusplus/afl-fuzz -Q -i ../inputs/storepng -o ../fuzz-native/ -- ../bin/storepng @@
