#ifndef BLOCK_H
#define BLOCK_H

#include <unistd.h>

#define FREE_DATA 2
#define BLOCK_SIZE 4096
#define FAILED -1

unsigned char *bread(int block_num, unsigned char *block);
void bwrite(int block_num, unsigned char *block);
int alloc(void);
off_t get_block_position(int block_num);

#endif