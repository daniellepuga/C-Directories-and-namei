// freeing blocks
#include "free.h"
#include "block.h"
#include <stdio.h>


// helper function to find lowest clear bit in a byte
int find_low_clear_bit(unsigned char x)
{
    for (int i = 0; i < BYTE; i++)
        if (!(x & (1 << i)))
            return i;
    
    return -1;
}

// set a specific bit to thev alue in set (0 or 1)
void set_free(unsigned char *block, int num, int set){
    int byte_num = num / BYTE;  // 8 bits per byte
    int bit_num = num % BYTE;

    if (set == 1) {
        block[byte_num] |= (set << bit_num);
    }
    else {
        block[byte_num] &= ~(1 << bit_num);
    }
}

// find a 0 bit and return its index
int find_free(unsigned char *block){
    for(int i = 0; i < BLOCK_SIZE; i++)
    {
        int bit_num = find_low_clear_bit(block[i]);
        if(bit_num != FAILED)
        {
            return (i * 8) + bit_num;
        }
    }
    return FAILED;
}