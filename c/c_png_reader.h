#include <stdint.h>
#include <stdio.h>
#include "linkedlist.h"

#define to_chunk_item(x) (chunk_item_t *)(x)

typedef struct chunk
{
    unsigned int length;
    unsigned int crc;
    unsigned char* type;
    unsigned char* data;
} chunk_t;

typedef struct ihdr
{
    unsigned int width;
    unsigned int height;
    unsigned int bit_depth;
    unsigned int color_type;
    unsigned int compression_method;
    unsigned int filter_method;
    unsigned int interlace_method;
} ihdr_t;

typedef struct chunk_item
{
    list_node_t node;
    chunk_t *chunk;
} chunk_item_t;

chunk_item_t *chunk_item_new(chunk_t *chunk);

int check_signature(FILE*);

int convert_little_endian_to_big(void*, int);

chunk_t* read_chunk(FILE*);

ihdr_t* processing_ihdr_chunk(chunk_t*);

unsigned char* processing_idat_chunk(chunk_item_t *);

int paeth_predictor(int, int, int);

int recon_a(unsigned char*, int, int, int, int);

int recon_b(unsigned char*, int, int, int, int);

int recon_c(unsigned char*, int, int, int, int);

unsigned char *reconstructing_pixel_data(unsigned char*, ihdr_t*);

unsigned char* get_pixel_data(const char*, int*, int*, int*);