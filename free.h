#ifndef FREE_H
#define FREE_H

#define BLOCK_SIZE 4096
#define BYTE 8

void set_free(unsigned char *block, int num, int set);
int find_free(unsigned char *block);

#endif