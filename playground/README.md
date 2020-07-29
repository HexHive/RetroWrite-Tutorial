# Playground that provides buggy binaries

In this directory you will find:

* bin/loadpng - a binary that exercises png reading code when supplied with a 
              single argument - the PNG file to read.
* bin/storepng - a binary that writes a png file based on a single provided 
              file as a source of random bytes (don't use /dev/urandom 
              directly for this, if you want test cases extract at least 1kb 
              of bytes from there).
* inputs/     - example valid inputs for the binaries.
* src/        - the source for the binaries, for your inspection (but we 
                don't rely on having this as part of the fuzzing campaign).


