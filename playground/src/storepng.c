
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pngparser.h"

#define SIZE_X 10
#define SIZE_Y 10
#define SIZE_PALETTE 10
// Load stub
//
int main(int argc, char* argv[])
{
  // Set up the image structures
  struct image img;
  struct pixel pixels[SIZE_Y][SIZE_X];
  struct pixel palette[SIZE_PALETTE] = {0};
  char randomByte;

  img.size_x = SIZE_X;
  img.size_y = SIZE_Y;

  img.px = (struct pixel*) pixels;

  if (argc != 2) {
    return 1;
  }
  
  // The output file will be generated based on the PID
  char outputFilename[256];
  snprintf(outputFilename, sizeof(outputFilename), "storepng-%d", getpid());
  
  // Input filename is passed as the first argument
  const char* filename = argv[1];
  
  FILE *f = fopen(filename, "rb");
  if (!f)
    return 1;

  fseek(f, 0, SEEK_END); // seek to end of file
  size_t size = ftell(f); // get current file pointer
  
  // We need enough data to fill the image, palette and generate a random byte
  if (size < 1000)
    return 1;

  // Reset file
  fseek(f, 0, SEEK_SET);

  // Read and fill random data
  fread(pixels, sizeof(pixels), 1, f);
  fread(palette, sizeof(palette), 1, f);
  fread(&randomByte, sizeof(randomByte), 1, f);
  fclose(f);
  
  // Call without or with palette
  if (randomByte & 0x01) {
      store_png(outputFilename, &img, NULL, 0);
  } else {
      store_png(outputFilename, &img, palette, SIZE_PALETTE);
  }

  return 0;
}
