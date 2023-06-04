// reading and writing blocks.
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "block.h"
#include "image.h"


// helper function to check block position
off_t get_block_position(int block_num){
    off_t offset = block_num * BLOCK_SIZE;
    off_t block_position = lseek(image_fd, offset, SEEK_SET);
    return block_position;
}

// allow us to read and write blocks.
// this function should take a block number and a pointer to a block
// sized unsigned char buffer to load the data into
unsigned char *bread(int block_num, unsigned char *block){
    off_t block_position = get_block_position(block_num);
    if (block_position == FAILED) {
        exit(1);
    }
    int read_bytes = read(image_fd, block, BLOCK_SIZE);
    if (read_bytes == FAILED) {
        exit(1);
    }
    return block;
}

// takes a block number and a pointer to the data to write.
void bwrite(int block_num, unsigned char *block){
    off_t block_position = get_block_position(block_num);
    if (block_position == FAILED) {
        exit(1);
    }
    int write_bytes = write(image_fd, block, BLOCK_SIZE);
    if (write_bytes == FAILED){
        exit(1);
    }
    // write(image_fd, block, BLOCK_SIZE);
    return;
}

// allocate a previous-free data block from the block map
int alloc(void){
   unsigned char block[BLOCK_SIZE];
   bread(FREE_DATA, block);
   int free_bit = find_free(block);
   if (free_bit != FAILED) {
        set_free(block, free_bit, 1);
        bwrite(FREE_DATA, block);
        
        return free_bit;
   } else {
        return FAILED;
   }
}