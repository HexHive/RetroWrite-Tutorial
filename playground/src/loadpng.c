
#include <stdio.h>
#include <stdlib.h>
#include "pngparser.h"

int main(int argc, char** argv) {
  struct image *test_img;

  if ( argc != 2 ) {
     printf("I need an input. Please provide a path to the PNG to load.\n");
     exit(1);
  }
  char* filename = argv[1];
  // What would happen if we run multiple fuzzing processes at the same time?
  // Take a look at the name of the file.
  load_png(filename, &test_img);

  // Always return 0
  return 0;
}
