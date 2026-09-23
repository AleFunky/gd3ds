#pragma once
#include <3ds.h>
u8 get_model();

size_t get_uncompressed_size(unsigned char *data, int data_len);
unsigned char *compress_data(const unsigned char *data, size_t data_len, size_t *out_len, int *out_code);
char *decompress_data(unsigned char *data, size_t data_len, size_t *out_len, int *out_code);


typedef struct GenericTask GenericTask;
typedef int (*GenericFunc)(GenericTask *);

typedef struct GenericTask {
    GenericFunc func;

    volatile int result;
    
    volatile bool running;
    volatile bool finished;
    volatile bool cancelled;
} GenericTask;

Thread create_generic_thread(GenericTask *task);