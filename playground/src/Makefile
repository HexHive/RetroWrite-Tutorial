

CC?=clang
CFLAGS=-fPIC `pkg-config --cflags --libs zlib`

%.o: %.c
	$(CC) -c $< -o $@

all: libpngparser.a loadpng storepng

clean:
	rm -f libpngparser.a loadpng storepng *.o

.PHONY: all clean s

libpngparser.a: pngparser.o crc.o 
	ar rcs libpngparser.a $^

loadpng: loadpng.o libpngparser.a
	$(CC) $(CFLAGS) -o $@ $^

storepng: storepng.o libpngparser.a
	$(CC) $(CFLAGS) -o $@ $^

