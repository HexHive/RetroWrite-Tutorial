#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


/* Each pixel consists of four channels: RGBA
 * 0 denotes black, while 0xff represents the maximum value the color can take
 * 
 * Alpha channel is a separate channel that denotes transparency.
 */
struct pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

/* The image comprises its dimensions and its pixels
 * Pixels are stored as an 1D array.
 *
 * The image:
 *
 * ABC
 * DEF
 * GHI
 * 
 * is linearized as: ABCDEFGHI
 * 
 * Pixel coordinates can take values [0, size_x - 1] and [0, size_y - 1]
 * 
 * We can access the pixel [y][x] by accessing the pixel at the index [y * size_x + x]
 * 
 * However, it is easier to use a C99 feature VLA (variable length arrays)
 * We will cast img->px into "struct image (*)[img->width_x]"
 * 
 * This will dereference the correct pixel, but the code using it will neeed to be in
 * a scope of its own (i.e. just add curly braces around it) if we want to use unstructured control flow
 * (i.e. goto statements).
 * 
 */

struct image {
    uint16_t size_x;
    uint16_t size_y;
    struct pixel *px;
};

/* load_png loads a png file denoted by filename and writes a pointer to struct image
 * into the memory pointed to by img.
 * 
 * Please remember to free both img and img->px after you are finished using the image.
 * Please also remember to do it in the correct order :)
 * 
 * This function returns 0 on success and a non-zero value on failure.
 */


int load_png(const char *filename, struct image **img);


/* store_png stores an image pointed to by img into a file whose name is passed as a filename argument
 * If the argument palette is NULL, the file will be stored in the RGBA format.
 * Otherwise, we will try to represent the image using the provided palette pixels. If this cannot be
 * accomplished, the function will fail.
 * 
 * This function returns 0 on success and a non-zero value on failure.
 */


int store_png(const char *filename, struct image *img, struct pixel *palette, uint8_t palette_length);

#endif
