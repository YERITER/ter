#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_FILENAME 260
#define MAX_FILES    1024
#define BLOCK_SIZE   8192
#define MAGIC_NUMBER "HUF2"

typedef struct HuffmanNode {
    unsigned char data;
    unsigned int freq;
    struct HuffmanNode* left, * right;
} HuffmanNode;

typedef struct {
    unsigned int code;
    unsigned char len;
} HuffmanCode;

typedef struct {
    char filename[MAX_FILENAME];
    unsigned long long orig_size;
    unsigned long long comp_size;
    unsigned long long offset;
} FileInfo;

typedef struct {
    char magic[4];
    unsigned int file_cnt;
    unsigned int freq[256];
} HufHeader;

HuffmanNode* create_tree(unsigned int* freq);
void build_codes(HuffmanNode* root, HuffmanCode* tab);
void free_tree(HuffmanNode* root);

int compress_one(const char* in, const char* out);
int decompress_one(const char* in, const char* out);

int compress_multi(const char** files, int cnt, const char* out);
int decompress_multi(const char* in, const char* outdir);

unsigned long long file_size(const char* fn);
bool file_exists(const char* fn);
void mk_dir(const char* path);
void bench(const char* fn);

#endif