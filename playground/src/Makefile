

CC?=clang
CFLAGS=-fPIC -O2 -fPIE
LDFLAGS=-fPIC -fPIE -pie `pkg-config --cflags --libs zlib`

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: libpngparser.a loadpng storepng

clean:
	rm -f libpngparser.a loadpng storepng *.o

.PHONY: all clean s

libpngparser.a: pngparser.o crc.o 
	ar rcs libpngparser.a $^

loadpng: loadpng.o libpngparser.a
	$(CC) $(LDFLAGS) -o $@ $^

storepng: storepng.o libpngparser.a
	$(CC) $(LDFLAGS) -o $@ $^

