
retrowrite loadpng loadpng_symb.s
AFL_AS_FORCE_INSTRUMENT=1 $AFL_PATH/afl-gcc loadpng_symb.s -o loadpng_symb_inst -lz
retrowrite storepng storepng_symb.s
AFL_AS_FORCE_INSTRUMENT=1 $AFL_PATH/afl-gcc storepng_symb.s -o storepng_symb_inst -lz

