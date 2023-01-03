#include "zlib.h"   // for printf
#include <string.h> // for memset
#include <stdlib.h> // for malloc/free
#include "c_png_reader.h"

#define ZLIB_BUF_SIZE 16 * 1024

const char *png_signature = "\x89PNG\r\n\x1a\n";

chunk_item_t *chunk_item_new(chunk_t *chunk)
{
    chunk_item_t *item = malloc(sizeof(chunk_item_t));
    if (!item)
    {
        return NULL;
    }
    item->chunk = chunk;

    return item;
}

int check_signature(FILE *fp)
{
    char signature[8];
    size_t size = fread(&signature, 8, 1, fp);
    if (memcmp(signature, png_signature, 8) != 0)
    {
        fprintf(stderr, "Invalid PNG signature\n");
        return 0;
    }
    return 1;
}

int convert_little_endian_to_big(void *value_to_convert, int length)
{
    int big_endian;
    char *big_endian_ptr = (char *)&big_endian;
    char *little_endian_ptr = (char *)value_to_convert;
    for (size_t i = 0; i < length; i++)
    {
        big_endian_ptr[i] = little_endian_ptr[length - 1 - i];
    }
    return big_endian;
}

chunk_t *read_chunk(FILE *file)
{
    chunk_t *chunk = malloc(sizeof(chunk_t));
    if (!chunk)
    {
        return NULL;
    }

    // get chunk length
    fread(&chunk->length, 4, 1, file);
    chunk->length = convert_little_endian_to_big(&chunk->length, 4);
    printf("chunk length: %d\n", chunk->length);

    // get chunk type
    chunk->type = malloc(4);
    if (!chunk->type)
    {
        free(chunk);
        return NULL;
    }
    fread(chunk->type, 4, 1, file);
    printf("chunk type: %s\n", chunk->type);

    // get chunk data
    chunk->data = malloc(chunk->length);
    if (!chunk->data)
    {
        free(chunk);
        return NULL;
    }
    fread(chunk->data, chunk->length, 1, file);
    printf("chunk data: %s\n", chunk->data);

    // get CRC
    fread(&chunk->crc, 4, 1, file);
    chunk->crc = convert_little_endian_to_big(&chunk->crc, 4);
    unsigned int checksum = crc32(crc32(0, chunk->type, 4), chunk->data, chunk->length);
    if (chunk->crc != checksum)
    {
        printf("chunk checksum failed %d != %d\n", chunk->crc, checksum);
        free(chunk);
        return NULL;
    }
    printf("chunk crc: %x\n", chunk->crc);
    return chunk;
}

ihdr_t *processing_ihdr_chunk(chunk_t *chunk)
{
    unsigned char *data = malloc(4);
    ihdr_t *ihdr_data = malloc(sizeof(ihdr_t));

    // get width
    for (size_t i = 0; i < 4; i++)
    {
        data[i] = chunk->data[i];
    }
    ihdr_data->width = convert_little_endian_to_big(data, 4);

    // get height
    for (size_t i = 0; i < 4; i++)
    {
        data[i] = chunk->data[i + 4];
    }
    ihdr_data->height = convert_little_endian_to_big(data, 4);

    ihdr_data->bit_depth = chunk->data[8];
    ihdr_data->color_type = chunk->data[9];
    ihdr_data->compression_method = chunk->data[10];
    ihdr_data->filter_method = chunk->data[11];
    ihdr_data->interlace_method = chunk->data[12];

    // sanity checking
    if (ihdr_data->compression_method != 0)
    {
        printf("invalid compression method %u\n", ihdr_data->compression_method);
        free(ihdr_data);
        free(data);
        return NULL;
    }
    if (ihdr_data->filter_method != 0)
    {
        printf("invalid filter method\n");
        free(ihdr_data);
        free(data);
        return NULL;
    }
    if (ihdr_data->color_type != 6)
    {
        printf("we only support truecolor with alpha\n");
        free(ihdr_data);
        free(data);
        return NULL;
    }
    if (ihdr_data->bit_depth != 8)
    {
        printf("we only support a bit depth of 8\n");
        free(ihdr_data);
        free(data);
        return NULL;
    }
    if (ihdr_data->interlace_method != 0)
    {
        printf("we only support no interlacing\n");
        free(ihdr_data);
        free(data);
        return NULL;
    }

    printf("width: %u\n", ihdr_data->width);
    printf("heigth: %u\n", ihdr_data->height);

    free(data);
    return ihdr_data;
}

unsigned char *processing_idat_chunk(chunk_item_t *chunk_list)
{
    chunk_item_t *chunk_copy = chunk_list;
    int max_size = 0;
    while (chunk_copy)
    {
        if (memcmp("IDAT", chunk_copy->chunk->type, 4) == 0)
        {
            max_size += chunk_copy->chunk->length;
        }
        if (memcmp("IEND", chunk_copy->chunk->type, 4) == 0)
        {
            break;
        }
        chunk_copy = to_chunk_item(chunk_copy->node.next);
    }
    
    printf("%s: %d\n", "maxsize", max_size);

    unsigned char *data = malloc(max_size);
    int counter = 0;
    for (;;)
    {
        if (memcmp("IDAT", chunk_list->chunk->type, 4) == 0)
        {
            for (size_t i = 0; i < chunk_list->chunk->length; i++)
            {
                data[counter++] = chunk_list->chunk->data[i];
            }
        }
        if (memcmp("IEND", chunk_list->chunk->type, 4) == 0)
        {
            break;
        }
        chunk_list = to_chunk_item(chunk_list->node.next);
    }

    if (!data)
    {
        printf("IDAT not found\n");
        return NULL;
    }

    return data;
}

int paeth_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }
    else
    {
        return c;
    }
}

int recon_a(unsigned char *pixels, int r, int c, int bytes_per_pixel, int stride)
{
    if (c >= bytes_per_pixel)
    {
        return pixels[r * stride + c - bytes_per_pixel];
    }
    return 0;
}

int recon_b(unsigned char *pixels, int r, int c, int bytes_per_pixel, int stride)
{
    if (r > 0)
    {
        return pixels[(r - 1) * stride + c];
    }
    return 0;
}

int recon_c(unsigned char *pixels, int r, int c, int bytes_per_pixel, int stride)
{
    if (r > 0 && c >= bytes_per_pixel)
    {
        return pixels[(r - 1) * stride + c - bytes_per_pixel];
    }
    return 0;
}

unsigned char *reconstructing_pixel_data(unsigned char *data, ihdr_t *ihdr_data)
{
    int bytes_per_pixel = 4;
    int stride = ihdr_data->width * bytes_per_pixel;

    // decompress IDAT chunk
    unsigned long compressed_max_size = compressBound(ihdr_data->height * (1 + stride));
    printf("compressed max_size %lu\n", compressed_max_size);

    unsigned long uncompressed_size = ihdr_data->height * (1 + stride);
    printf("uncompressed size %lu\n", uncompressed_size);

    unsigned char *uncompressed_data = malloc(uncompressed_size);

    if (!uncompressed_data)
    {
        return NULL;
    }

    int result = uncompress(uncompressed_data, &uncompressed_size, data, compressed_max_size);

    if (result != Z_OK)
    {
        printf("unable to uncompress: error %d\n", result);
        free(uncompressed_data);
        return NULL;
    }
    printf("result: %d\n", result);

    // assign pixels memory
    unsigned char *pixels = malloc(ihdr_data->height * stride);
    if (!pixels)
    {
        printf("pixel error\n");
        free(uncompressed_data);
        return NULL;
    }

    int i = 0;
    int count = 0;
    int recon_x = 0;
    for (size_t r = 0; r < ihdr_data->height; r++)
    {
        int filter_type = uncompressed_data[i];
        i += 1;
        for (size_t c = 0; c < stride; c++)
        {
            int filt_x = uncompressed_data[i];
            i += 1;
            if (filter_type == 0)
            {
                recon_x = filt_x;
            }
            else if (filter_type == 1)
            {
                recon_x = filt_x + recon_a(pixels, r, c, bytes_per_pixel, stride);
            }
            else if (filter_type == 2)
            {
                recon_x = filt_x + recon_b(pixels, r, c, bytes_per_pixel, stride);
            }
            else if (filter_type == 3)
            {
                recon_x = filt_x + (recon_a(pixels, r, c, bytes_per_pixel, stride) + recon_b(pixels, r, c, bytes_per_pixel, stride)) / 2;
            }
            else if (filter_type == 4)
            {
                recon_x = filt_x + paeth_predictor(recon_a(pixels, r, c, bytes_per_pixel, stride), recon_b(pixels, r, c, bytes_per_pixel, stride), recon_c(pixels, r, c, bytes_per_pixel, stride));
            }
            else
            {
                printf("unknown filter type %d\n", filter_type);
            }

            pixels[count++] = recon_x;
        }
    }

    return pixels;
}

unsigned char *get_pixel_data(const char *filename, int *width, int *heigth, int *channels)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return NULL;
    }

    if (!check_signature(file))
    {
        return NULL;
    }

    chunk_item_t *chunk_list = NULL;
    // read chunks
    for (;;)
    {
        chunk_t *chunk = read_chunk(file);
        list_append(to_list(&chunk_list), to_list_node(chunk_item_new(chunk)));
        if (memcmp("IEND", chunk->type, 4) == 0)
        {
            break;
        }
    }

    // processing ihdr
    ihdr_t *ihdr_data = processing_ihdr_chunk(chunk_list->chunk);

    // processing idat
    unsigned char* data = processing_idat_chunk(chunk_list);

    unsigned char *pixels = reconstructing_pixel_data(data, ihdr_data);
    if (!pixels)
    {
        printf("pixel error\n");
        free(chunk_list);
        return NULL;
    }

    *width = ihdr_data->width;
    *heigth = ihdr_data->height;
    *channels = 4;

    free(chunk_list);
    fclose(file);
    return pixels;
}