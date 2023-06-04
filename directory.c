#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "directory.h"
#include "block.h"
#include "mkfs.h"
#include "pack.h"

struct directory *directory_open(int inode_num)
{
    // use iget() to get inode for file
    struct inode *directory_inode = iget(inode_num);
    // if it fails, directory_open() should return NULL
    if (directory_inode == NULL) {
        return NULL;
    }
    // malloc space for a new struct direcotry
    struct directory *open_directory = malloc(sizeof(struct directory));
    // set the inode pointer to point to the inode returned by iget()
    open_directory->inode = directory_inode;
    open_directory->offset = 0;
    // return the point to the struct
    return open_directory;
}

// reading a dictionary
int directory_get(struct directory *dir, struct directory_entry *ent)
{
    // check the offset against the size of directory
    unsigned int offset = dir->offset;
    unsigned int size = dir->inode->size;
    // if offset greater than or equal to directory size, return -1 to
    // indicate we are at the end
    if (offset >= size) {
        return -1;
    }
    // computer the block in the directory we need to read
    int data_block_index = offset / BLOCK_SIZE;
    // from project spec
    short data_block_num = dir->inode->block_ptr[data_block_index] / BLOCK_SIZE;
    // read to the appropriate data block before giving it to bread()
    unsigned char block[BLOCK_SIZE];
    bread(data_block_num, block);
    // Calculate the offset within the block
    int offset_in_block = offset % BLOCK_SIZE;
    // read to extract the inode number and store it in ent->inode_num
    ent->inode_num = read_u16(block + offset_in_block);
    // copy the file name and store it in ent-> name
    strcpy(ent->name, (char *)block + FILE_OFFSET);

    return 0;
}

// closing the directory
void directory_close(struct directory *d)
{
    iput(d->inode);
    free(d);
}