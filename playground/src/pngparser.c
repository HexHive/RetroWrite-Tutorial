#include "zlib.h"
#include "crc.h"
#include "pngparser.h"
#include <stdlib.h>
#include <string.h>

#define PNG_OUTPUT_CHUNK_SIZE (1 << 14)

#define PNG_IHDR_COLOR_GRAYSCALE 0
#define PNG_IHDR_COLOR_RGB 2
#define PNG_IHDR_COLOR_PALETTE 3
#define PNG_IHDR_COLOR_GRAYSCALE_ALPHA 4
#define PNG_IHDR_COLOR_RGB_ALPHA 6

#define PNG_IHDR_INTERLACE_NO_INTERLACE 0
#define PNG_IHDR_INTERLACE_ADAM7 1

#define to_little_endian(x) (change_endianness(x))
#define to_big_endian(x) (change_endianness(x))

/** Changes the endianness of the data
*/
uint32_t change_endianness(uint32_t x)
{
    int i;
    uint32_t result = 0;

    for (i = 0; i < 4; i++)
    {
        result |= ((x >> (8*i)) & 0xff) << (8 * (3 - i));
    }

    return result; 
}

/** 
 * Y0l0 PNG data is divided into chunks. Every chunk consists of:
 * 1) the length (in bytes)
 * 2) a code denoting the type of the chunk
 * 3) the chunk data
 * 4) CRC checksum
*/
struct __attribute__((__packed__))  png_chunk
{
    uint32_t length;
    uint32_t chunk_type;
    void * chunk_data;
    uint32_t crc;
};

/**
 * Different chunk types
 */
typedef struct png_chunk png_chunk_ihdr;
typedef struct png_chunk png_chunk_plte;
typedef struct png_chunk png_chunk_idat;
typedef struct png_chunk png_chunk_iend;

/* The header that carries image metadata
 */
struct __attribute__((__packed__)) png_header_ihdr
{
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression;
    uint8_t filter;
    uint8_t interlace;
};

/* Color description in the palette
*/
struct __attribute__((__packed__))  plte_entry
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/* Starting bytes of an image
*/
struct __attribute__((__packed__)) png_header_filesig
{
    uint8_t filesig[8];
};

/* The current implementation supports only some of the PNG color types
*/
int is_color_type_valid(uint8_t color_type)
{
    switch (color_type)
    {
        
        case PNG_IHDR_COLOR_PALETTE:
        case PNG_IHDR_COLOR_RGB_ALPHA:
            return 1;
        case PNG_IHDR_COLOR_GRAYSCALE:
        case PNG_IHDR_COLOR_RGB:
        case PNG_IHDR_COLOR_GRAYSCALE_ALPHA:
        default:
            return 0;
    }
}

/* Currently, we support only some of the bidepths */
int is_bit_depth_valid(uint8_t color_type, int8_t bitdepth)
{
    if (color_type == PNG_IHDR_COLOR_PALETTE &&  bitdepth == 8)
        return 1;

    if (color_type == PNG_IHDR_COLOR_RGB_ALPHA && bitdepth == 8)
        return 1;

    return 0;
}

/* The only supported method is deflate */
int is_compression_valid(uint8_t compression)
{
    return !compression;
}

/* We support only the default PNG filtering method
 * Filtering is a step before compression.
 * Compression may be better if we store differences between pixels
 * instead of the actual pixel values.
 */
int is_filter_valid(uint8_t filter)
{
    return !filter;
}

/* The only filter type that we support is no filter at all */
int is_filter_type_valid(uint8_t filter_type)
{
    return !filter_type;
}

/* Y0L0 PNG stores all of its data as a sequence of rows.
 * Some other, barbaric standards (e.g. PNG) also provide storage sequences
 * that give lower resolution approximations of the image while streaming the data
 */
int is_interlace_valid(uint8_t interlace)
{
    switch (interlace)
    {
        case PNG_IHDR_INTERLACE_NO_INTERLACE:
            return 1;
            
        // We don't support interlacing
        //
        case PNG_IHDR_INTERLACE_ADAM7:
            return 0;

        default:
            return 0;
    }
}


/* Check if the metadata header is valid */
int is_png_ihdr_valid(struct png_header_ihdr *ihdr)
{
    if (!is_color_type_valid(ihdr->color_type))
        return 0;

    if (!is_bit_depth_valid(ihdr->color_type, ihdr->bit_depth))
        return 0;

    if (!is_compression_valid(ihdr->compression))
        return 0;

    if (!is_filter_valid(ihdr->filter))
        return 0;

    if (!is_interlace_valid(ihdr->interlace))
        return 0;

    return 1;
}

/* Check if we have the IHDR chunk */
int is_chunk_ihdr(struct png_chunk *chunk)
{
    return !memcmp(&chunk->chunk_type, "IHDR", 4);
}

/* Convert a freshly read generic chunk to IHDR */
png_chunk_ihdr *format_ihdr_chunk(struct png_chunk *chunk)
{
    png_chunk_ihdr *ihdr;
    struct png_header_ihdr *ihdr_header;

    if (!is_chunk_ihdr(chunk))
        return NULL;

    if (chunk->length != sizeof(struct png_header_ihdr))
        return NULL;

    ihdr = (png_chunk_ihdr *) chunk;

    if (!is_png_ihdr_valid(ihdr->chunk_data))
        return NULL;

    ihdr_header = (struct png_header_ihdr *) ihdr->chunk_data;
    ihdr_header->height = to_little_endian(ihdr_header->height);
    ihdr_header->width  = to_little_endian(ihdr_header->width);

    return ihdr;
}

/* Check if this is the IEND chunk */
int is_chunk_iend(struct png_chunk *chunk)
{
    return !memcmp(&chunk->chunk_type, "IEND", 4);
}

/* Format a freshly read chunk as an IEND chunk */
png_chunk_iend *format_iend_chunk(struct png_chunk *chunk)
{
    if (!is_chunk_iend(chunk)) {
        return NULL;
    }

    if (chunk->length) {
        return NULL;
    }

    if (chunk->chunk_data) {
        return NULL;
    }

    return (png_chunk_iend *) chunk;
}

/* Read the signature of a file */
int read_png_filesig(FILE *file, struct png_header_filesig *filesig)
{
    return fread(filesig, sizeof(*filesig), 1, file) != 1;
}

/* Checks if the first bytes have the correct values */
int is_png_filesig_valid(struct png_header_filesig *filesig)
{
    return !memcmp(filesig, "\211PNG\r\n\032\n", 8);
}

/* CRC for a chunk. This prevents data corruption.
 *
 * EDIT THIS FUNCTION BEFORE FUZZING!
 */
int is_png_chunk_valid(struct png_chunk *chunk)
{
    /*uint32_t crc_value = crc((unsigned char *) &chunk->chunk_type, sizeof(int32_t));

    if (chunk->length) {
        crc_value = update_crc(crc_value ^ 0xffffffffL, (unsigned char *) chunk->chunk_data, chunk->length) ^ 0xffffffffL;
    }

    return chunk->crc == crc_value;*/
	return 1;
}

/* Fill the chunk with the data from the file.*/
int read_png_chunk(FILE *file, struct png_chunk *chunk)
{
    chunk->chunk_data = NULL;

    if (fread(chunk, sizeof(int32_t), 2, file) != 2) {
        goto error;
    }

    chunk->length = to_little_endian(chunk->length);

    if (chunk->length) {
        chunk->chunk_data = malloc(chunk->length);
        if (!chunk->chunk_data)
            goto error;

        if (fread(chunk->chunk_data, chunk->length, 1, file) != 1) {
            goto error;
        }
    }

    if (fread(&chunk->crc, sizeof(int32_t), 1, file) != 1) {
        goto error;
    }

    chunk->crc = to_little_endian(chunk->crc);

    if (!is_png_chunk_valid(chunk)) {
        goto error;
    }

    chunk->chunk_type = chunk->chunk_type;

    return 0;

error:
    if (chunk->chunk_data) {
        free(chunk->chunk_data);
        chunk->chunk_data = NULL;
    }
    return 1;
}

/* Does the chunk represent a palette of colors?*/
int is_chunk_plte(struct png_chunk *chunk)
{
    return !memcmp(&chunk->chunk_type, "PLTE", 4);
}

/* Reinterpret a chunk to the PLTE chunk, if possible */
png_chunk_plte *format_plte_chunk(struct png_chunk *chunk)
{
    if (!is_chunk_plte(chunk))
        return NULL;

    if (chunk->length % 3)
        return NULL;

    return (png_chunk_plte *) chunk;
}

/* Does the chunk represent image data? */
int is_chunk_idat(struct png_chunk *chunk)
{
    return !memcmp(&chunk->chunk_type, "IDAT", 4);
}

/* Query the metadata for the interlacing type */
int is_interlaced(png_chunk_ihdr *ihdr_chunk)
{
    struct png_header_ihdr *ihdr_header = (struct png_header_ihdr *) ihdr_chunk->chunk_data;
    return ihdr_header->interlace;
}

/* Reinterpret a generic chunk as an IDAT chunk */
png_chunk_idat *format_idat_chunk(struct png_chunk *chunk) {
    if (!is_chunk_idat(chunk)) {
        return NULL;
    }

    return (png_chunk_idat *) chunk;
}

/* Take a deflate stream and decompress it. This is used to obtain image data from an IDAT train. */
int decompress_png_data(uint8_t *compressed_data, uint32_t input_length, uint8_t **decompressed_data, uint32_t *decompressed_length)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[PNG_OUTPUT_CHUNK_SIZE];
    uint8_t *output_buffer = NULL;
    uint32_t output_length = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = input_length;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;


    strm.next_in = compressed_data;

    /* run inflate() on input until output buffer not full */
    do {
        strm.avail_out = PNG_OUTPUT_CHUNK_SIZE;
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);

        switch (ret) {
        case Z_STREAM_ERROR:
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto error;
        }

        have = PNG_OUTPUT_CHUNK_SIZE - strm.avail_out;

        output_buffer = realloc(output_buffer, output_length + have);

        memcpy(output_buffer + output_length, out, have);

        output_length += have;
    } while (strm.avail_out == 0);

    if (ret != Z_STREAM_END) {
        goto error;
    }

    /* clean up and return */
    (void)inflateEnd(&strm);

    *decompressed_data = output_buffer;
    *decompressed_length = output_length;
    return 0;

error:
    inflateEnd(&strm);
    if (output_buffer) free(output_buffer);
    return 1;
}

/* Combine image metadata, palette and a decompressed image data buffer (with palette entries) into an image */
struct image *convert_color_palette_to_image(png_chunk_ihdr *ihdr_chunk, png_chunk_plte *plte_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    struct png_header_ihdr *ihdr_header = (struct png_header_ihdr *) ihdr_chunk->chunk_data;
    uint32_t height = ihdr_header->height;
    uint32_t width = ihdr_header->width;
    uint32_t palette_idx = 0;

    struct plte_entry *plte_entries = (struct plte_entry *) plte_chunk->chunk_data;

    struct image * img = malloc(sizeof(struct image));
    img->size_y = height;
    img->size_x = width;
    img->px = malloc(sizeof(struct pixel) * img->size_x * img->size_y);

    for (uint32_t idy = 0; idy < height; idy++) {
        // Filter byte at the start of every scanline needs to be 0
        if (inflated_buf[idy * (1 + width)]) {
            free(img->px);
            free(img);
            return NULL;
        }
        for (uint32_t idx = 0; idx < width; idx++) {
            palette_idx = inflated_buf[idy * (1 + width) + idx + 1];
            img->px[idy * img->size_x + idx].red = plte_entries[palette_idx].red;
            img->px[idy * img->size_x + idx].green = plte_entries[palette_idx].green;
            img->px[idy * img->size_x + idx].blue = plte_entries[palette_idx].blue;
            img->px[idy * img->size_x + idx].alpha = 0xff;
        }
    }

    return img;
}

/* Combine image metadata and decompressed image data (RGBA) into an image */
struct image *convert_rgb_alpha_to_image(png_chunk_ihdr *ihdr_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    struct png_header_ihdr *ihdr_header = (struct png_header_ihdr *) ihdr_chunk->chunk_data;
    uint32_t height = ihdr_header->height;
    uint32_t width = ihdr_header->width;

    uint32_t pixel_idx = 0;
    uint32_t r_idx, g_idx, b_idx, a_idx;

    struct image * img = malloc(sizeof(struct image));
    
    if (!img) {
        goto error;
    }

    img->size_y = height;
    img->size_x = width;
    img->px = malloc(sizeof(struct pixel) * img->size_x * img->size_y);

    if (!img->px) {
        goto error;
    }

    for (uint32_t idy = 0; idy < height; idy++) {
        // The filter byte at the start of every scanline needs to be 0
        if (inflated_buf[idy * (1 + 4 * width)]) {
            goto error;
        }

        for (uint32_t idx = 0; idx < width; idx++) {
            pixel_idx = idy * (1 + 4 * width) + 1 + 4*idx;

            r_idx = pixel_idx;
            g_idx = pixel_idx + 1;
            b_idx = pixel_idx + 2;
            a_idx = pixel_idx + 3;

            img->px[idy * img->size_x + idx].red = inflated_buf[r_idx];
            img->px[idy * img->size_x + idx].green = inflated_buf[g_idx];
            img->px[idy * img->size_x + idx].blue = inflated_buf[b_idx];
            img->px[idy * img->size_x + idx].alpha = inflated_buf[a_idx];
        }
    }

    return img;

error:
    if (img) {
        if (img->px) {
            free(img->px);
        }
        free(img);
    }
}

/* Creates magic unicorns */
void reverse_filter_on_scanlines(png_chunk_ihdr *ihdr_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    return;
}

/* Dispatch function for converting decompressed data into an image */
struct image *convert_data_to_image(png_chunk_ihdr *ihdr_chunk, png_chunk_plte *plte_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    struct png_header_ihdr *ihdr_header = (struct png_header_ihdr *) ihdr_chunk->chunk_data;
    switch (ihdr_header->color_type)
    {
        case PNG_IHDR_COLOR_PALETTE:
            return convert_color_palette_to_image(ihdr_chunk, plte_chunk, inflated_buf, inflated_size);
        case PNG_IHDR_COLOR_RGB_ALPHA:
            return convert_rgb_alpha_to_image(ihdr_chunk, inflated_buf, inflated_size);
        default:
            return NULL;
    }
}

/* Parses a Y0L0 PNG with no interlacing */
struct image *parse_png_no_interlace(png_chunk_ihdr *ihdr_chunk, png_chunk_plte *plte_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    reverse_filter_on_scanlines(ihdr_chunk, inflated_buf, inflated_size);

    return convert_data_to_image(ihdr_chunk, plte_chunk, inflated_buf, inflated_size);
}

/* Parses a Y0L0 PNG from read data. Returns NULL if the image is interlaced. */
struct image *parse_png(png_chunk_ihdr *ihdr_chunk, png_chunk_plte *plte_chunk, uint8_t *inflated_buf, uint32_t inflated_size)
{
    if (!ihdr_chunk) {
        return NULL;
    }

    struct png_header_ihdr *ihdr_header = (struct png_header_ihdr *) ihdr_chunk->chunk_data;

    switch (ihdr_header->interlace) {
        case PNG_IHDR_INTERLACE_NO_INTERLACE:
            return parse_png_no_interlace(ihdr_chunk, plte_chunk, inflated_buf, inflated_size);
        case PNG_IHDR_INTERLACE_ADAM7:
        default:
            return NULL;
    }
}

/* Reads a Y0l0 PNG from file and parses it into an image */
int load_png(const char *filename, struct image **img)
{
    struct png_header_filesig filesig;
    png_chunk_ihdr *ihdr_chunk = NULL;
    png_chunk_plte *plte_chunk = NULL;
    png_chunk_iend *iend_chunk = NULL;
    
    uint32_t deflated_data_length = 0;
    uint32_t deflated_data_idx = 0;
    uint8_t *deflated_buf = NULL;

    uint8_t *inflated_buf = NULL;
    uint32_t inflated_size = 0;

    int idat_train_started = 0;
    int idat_train_finished = 0;

    int chunk_idx = -1;

    struct png_chunk *current_chunk = malloc(sizeof(struct png_chunk));

    // libFuzzer example bug demo:
    current_chunk->chunk_data = NULL;

    FILE *input = fopen(filename, "rb");

    // Has the file been open properly?
    if (!input) {
        goto error;
    }

    // Did we read the starting bytes properly?
    if (read_png_filesig(input, &filesig)) {
        goto error;
    }

    // Are the starting bytes correct?
    if (!is_png_filesig_valid(&filesig)) {
        goto error;
    }

    // Read all PNG chunks
    for (;!read_png_chunk(input, current_chunk); current_chunk = malloc(sizeof(struct png_chunk)))
    {
        chunk_idx++;
        // We have more chunks after IEND for some reason
        // IEND must be the last chunk
        if (iend_chunk)
            goto error;

        // All IDAT chunks need to occur in sequence
        // We end the IDAT sequence here if we encounter a different chunk
        if (idat_train_started && !is_chunk_idat(current_chunk)) {
            idat_train_finished = 1;
            idat_train_started = 0;
        }

        // The first iteration: We must have IHDR!
        if (!chunk_idx) {
            if (!is_chunk_ihdr(current_chunk)) {
                goto error;
            }
        }

        if (is_chunk_ihdr(current_chunk)) {
            // The second IHDR?
            if (ihdr_chunk) {
                goto error;
            }

            ihdr_chunk = format_ihdr_chunk(current_chunk);

            if (!ihdr_chunk) {
                goto error;
            }

            continue;
        }
        
        // PLTE chunk encountered
        if (is_chunk_plte(current_chunk)) {
            // Only 1 PLTE is allowed
            if (plte_chunk) {
                goto error;
            }

            plte_chunk = format_plte_chunk(current_chunk);

            if (!plte_chunk) {
                goto error;
            }

            continue;
        }

        // IEND chunk
        if (is_chunk_iend(current_chunk)) {
            iend_chunk = format_iend_chunk(current_chunk);

            if (!iend_chunk) {
                goto error;
            }

            continue;
        }

        // Aggregate all IDAT data together before decompressing
        if (is_chunk_idat(current_chunk)) {
            png_chunk_idat *idat_chunk;

            // If we have already processed a sequence of IDATs, why do we see another one here?
            if (idat_train_finished) {
                goto error;
            }
            idat_train_started = 1;

            idat_chunk = format_idat_chunk(current_chunk);

            deflated_data_length += idat_chunk->length;
            deflated_buf = realloc(deflated_buf, deflated_data_length);

            memcpy(deflated_buf + deflated_data_idx, idat_chunk->chunk_data, idat_chunk->length);

            if (idat_chunk->chunk_data) {
                free(idat_chunk->chunk_data);
            }

            free(idat_chunk);
        }
    }

    // After we finish looping, we should have processed IEND
    if (!iend_chunk) {
        goto error;
    }

    // Decompress IDAT data
    if (decompress_png_data(deflated_buf, deflated_data_length, &inflated_buf, &inflated_size)) {
        goto error;
    }

    // Process decompressed data
    *img = parse_png(ihdr_chunk, plte_chunk, inflated_buf, inflated_size);

    if (!*img) {
        goto error;
    }

 success:
    fclose(input);

    if (deflated_buf) free(deflated_buf);

    if (current_chunk) {
        if (current_chunk->chunk_data) {
            free(current_chunk->chunk_data);
        }
        free(current_chunk);
    }

    if (plte_chunk) free(plte_chunk);

    if (ihdr_chunk) free(ihdr_chunk);

    return 0;
error:
    fclose(input);

    if (deflated_buf) free(deflated_buf);

    if (current_chunk) {
        if (current_chunk->chunk_data) {
            // LibFuzzer says there is a bug here!
            // Let's fix it.
            free(current_chunk->chunk_data);
        }
        free(current_chunk);
    }

    if (plte_chunk) free(plte_chunk);

    if (ihdr_chunk) free(ihdr_chunk);

    return 1;
}

// Store a valid file signature
int store_filesig(FILE *output)
{
    return fwrite("\211PNG\r\n\032\n", 8, 1, output) != 1;
}

// Create a header for a RGBA image
struct png_header_ihdr fill_ihdr_rgb_alpha(struct image *img)
{
    struct png_header_ihdr ihdr;
    ihdr.bit_depth = 8;
    ihdr.color_type = PNG_IHDR_COLOR_RGB_ALPHA;
    ihdr.compression = 0;
    ihdr.filter = 0;
    ihdr.interlace = 0;
    ihdr.height = to_big_endian(img->size_y);
    ihdr.width = to_big_endian(img->size_x);

    return ihdr;
}

// Create a header for a PLTE image
struct png_header_ihdr fill_ihdr_plte(struct image *img)
{
    struct png_header_ihdr ihdr;
    ihdr.bit_depth = 8;
    ihdr.color_type = PNG_IHDR_COLOR_PALETTE;
    ihdr.compression = 0;
    ihdr.filter = 0;
    ihdr.interlace = 0;
    ihdr.height = to_big_endian(img->size_y);
    ihdr.width = to_big_endian(img->size_x);

    return ihdr;
}

// Expects the length in the little endian format and converts it to big endian
// Calculates and fills the CRC value for a chunk
void fill_chunk_crc(struct png_chunk *chunk)
{
    chunk->crc = crc((unsigned char *) &chunk->chunk_type, sizeof(chunk->chunk_type));
    if (chunk->length) {
        chunk->crc = update_crc(chunk->crc ^ 0xffffffffL, (unsigned char *) chunk->chunk_data, chunk->length) ^ 0xffffffffL;
    }
    chunk->crc = to_big_endian(chunk->crc);
    chunk->length = to_big_endian(chunk->length);
}

// Fills the IHDR image metadata chunk
png_chunk_ihdr fill_ihdr_chunk(struct png_header_ihdr *ihdr)
{
    png_chunk_ihdr ihdr_chunk;
    memcpy(&ihdr_chunk.chunk_type, "IHDR", 4);
    ihdr_chunk.length = sizeof(*ihdr);
    ihdr_chunk.chunk_data = ihdr;
    fill_chunk_crc((struct png_chunk *) &ihdr_chunk);
    return ihdr_chunk;
}

// Chunk needs to already be in the big endian format
// Writes a chunk to the file
int store_png_chunk(FILE *output, struct png_chunk *chunk)
{
    fwrite(&chunk->length, 4, 1, output);
    fwrite(&chunk->chunk_type, 4, 1, output);
    fwrite(chunk->chunk_data, to_little_endian(chunk->length), 1, output);
    fwrite(&chunk->crc, 4, 1, output);
}

// Writes a RGBA metadata chunk
int store_ihdr_rgb_alpha(FILE *output, struct image *img)
{
    struct png_header_ihdr ihdr = fill_ihdr_rgb_alpha(img);
    png_chunk_ihdr ihdr_chunk = fill_ihdr_chunk(&ihdr);
    store_png_chunk(output, (struct png_chunk *) &ihdr_chunk);
    return 0;
}

// Writes a palette metadata chunk
int store_ihdr_plte(FILE *output, struct image *img)
{
    struct png_header_ihdr ihdr = fill_ihdr_plte(img);
    png_chunk_ihdr ihdr_chunk = fill_ihdr_chunk(&ihdr);
    store_png_chunk(output, (struct png_chunk *) &ihdr_chunk);
    return 0;
}

// Compresses image data using deflate
int compress_png_data(uint8_t *decompressed_data, uint32_t decompressed_length, uint8_t **compressed_data, uint32_t *compressed_length)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char out[PNG_OUTPUT_CHUNK_SIZE];
    int level = 1;

    *compressed_data = NULL;
    *compressed_length = 0;
    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    
    strm.avail_in = decompressed_length;
    flush = Z_FINISH;
    strm.next_in = decompressed_data;

    /* run deflate() on input until output buffer not full, finish
        compression if all of source has been read in */
    do {
        strm.avail_out = PNG_OUTPUT_CHUNK_SIZE;
        strm.next_out = out;
        ret = deflate(&strm, flush);    /* no bad return value */
        if (ret == Z_STREAM_ERROR) {
            goto error;
        }
        have = PNG_OUTPUT_CHUNK_SIZE - strm.avail_out;

        *compressed_data = realloc(*compressed_data, *compressed_length + have);
        memcpy(*compressed_data + *compressed_length, out, have);
        *compressed_length += have;
        
    } while (strm.avail_out == 0);
    if (strm.avail_in != 0) {
        goto error;
    }

    if (ret != Z_STREAM_END) {
        goto error;
    }

    /* clean up and return */
    (void)deflateEnd(&strm);
    return 0;

error:
    if (*compressed_data) {
        free(*compressed_data);
    }
}

// Fills IDAT with compressed data
png_chunk_idat fill_idat_chunk(uint8_t *data, uint32_t length)
{
    png_chunk_idat idat;
    memcpy(&idat.chunk_type, "IDAT", 4);
    idat.chunk_data = data;
    idat.length = length;
    fill_chunk_crc(&idat);
    return idat;
}

// Writes an IDAT chunk from image data to a file
int store_idat_rgb_alpha(FILE *output, struct image *img)
{
    uint32_t non_compressed_length = img->size_y * (1 + img->size_x * 4);
    uint8_t *non_compressed_buf = malloc(non_compressed_length);

    for (uint32_t id_y = 0; id_y < img->size_y; id_y++) {
        non_compressed_buf[id_y * (1 + img->size_x * 4)] = 0;
        for (uint32_t id_x = 0; id_x < img->size_x; id_x++) {
            uint32_t id_pix_buf = id_y * (1 + img->size_x * 4) + 1 + 4*id_x;
            uint32_t id_pix = id_y * img->size_x + id_x;

            non_compressed_buf[id_pix_buf]     = img->px[id_pix].red;
            non_compressed_buf[id_pix_buf + 1] = img->px[id_pix].green;
            non_compressed_buf[id_pix_buf + 2] = img->px[id_pix].blue;
            non_compressed_buf[id_pix_buf + 3] = img->px[id_pix].alpha;
        }
    }

    uint8_t * compressed_data_buf;
    uint32_t compressed_length;

    compress_png_data(non_compressed_buf, non_compressed_length, &compressed_data_buf, &compressed_length);

    png_chunk_idat idat = fill_idat_chunk(compressed_data_buf, compressed_length);
    store_png_chunk(output, (struct png_chunk *) &idat);
    return 0;
}

// Finds a color in a palette and returns its index
int find_color(struct pixel *palette, uint32_t palette_length, struct pixel *target)
{
    for (int idx = 0; idx < palette_length; idx++) {
        if (palette[idx].red == target->red &&
            palette[idx].green == target->green &&
            palette[idx].blue == target->blue) {
                return idx;
            }
    }

    return -1;
}

// Writes an IDAT chunk for a palette image
int store_idat_plte(FILE *output, struct image *img, struct pixel *palette, uint32_t palette_length)
{
    uint32_t non_compressed_length = img->size_y * (1 + img->size_x);
    uint8_t *non_compressed_buf = malloc(non_compressed_length);

    for (uint32_t id_y = 0; id_y < img->size_y; id_y++) {
        non_compressed_buf[id_y * (1 + img->size_x)] = 0;
        for (uint32_t id_x = 0; id_x < img->size_x; id_x++) {
            uint32_t id_pix_buf = id_y * (1 + img->size_x) + 1 + id_x;
            uint32_t id_pix = id_y * img->size_x + id_x;
            int code = find_color(palette, palette_length, &img->px[id_pix]);
            if (code < 0) {
                goto error;
            }
            non_compressed_buf[id_pix_buf] = code;

        }
    }

    uint8_t * compressed_data_buf = NULL;
    uint32_t compressed_length;

    compress_png_data(non_compressed_buf, non_compressed_length, &compressed_data_buf, &compressed_length);

    png_chunk_idat idat = fill_idat_chunk(compressed_data_buf, compressed_length);
    store_png_chunk(output, (struct png_chunk *) &idat);
    return 0;

error:
    if (compressed_data_buf) {
        free(compressed_data_buf);
    }
    return 1;
}

// Writes the first two chunks for a RGBA image
int store_png_rgb_alpha(FILE *output, struct image *img)
{
    store_ihdr_rgb_alpha(output, img);
    store_idat_rgb_alpha(output, img);
}

// Creates a PLTE chunk from PLTE entries (colors)
png_chunk_plte fill_plte_chunk(struct plte_entry * plte_data, uint32_t color_count)
{
    png_chunk_plte plte;

    memcpy(&plte.chunk_type, "PLTE", 4);
    plte.chunk_data = plte_data;
    plte.length = 3 * color_count;
    fill_chunk_crc((struct png_chunk *) &plte);
    return plte;
}

// Writes a palette to the file
int store_plte(FILE *output, struct pixel *palette, uint32_t palette_length)
{
    struct plte_entry plte_data[256];

    for (int idx = 0; idx < palette_length; idx++) {
        plte_data[idx].red = palette[idx].red;
        plte_data[idx].green = palette[idx].green;
        plte_data[idx].blue = palette[idx].blue;
    }

    png_chunk_plte plte_chunk = fill_plte_chunk(plte_data, palette_length);
    store_png_chunk(output, (struct png_chunk *) &plte_chunk);
}

// Writes the first 3 chunks for a palette Y0L0 PNG image
int store_png_palette(FILE *output, struct image *img, struct pixel *palette, uint32_t palette_length)
{
    store_ihdr_plte(output, img);
    store_plte(output, palette, palette_length);
    store_idat_plte(output, img, palette, palette_length);
}

// Stores an IEND chunk to a file
int store_png_chunk_iend(FILE *output)
{
    png_chunk_iend iend;
    memcpy(&iend.chunk_type,"IEND", 4);
    iend.length = 0;
    fill_chunk_crc(&iend);
    store_png_chunk(output, &iend);
}

// Store a Y0L0 PNG to a file. Provide an array of pixels if you want to use a palette format.
// If it is NULL, RGBA is selected.
int store_png(const char *filename, struct image *img, struct pixel *palette, uint8_t palette_length)
{
    int result = 0;
    FILE *output = fopen(filename, "wb");

    store_filesig(output);

    if (palette) {
        store_png_palette(output, img, palette, palette_length);
    } else {
        store_png_rgb_alpha(output, img);
    }

    store_png_chunk_iend(output);
    fclose(output);
    return 0;
}
