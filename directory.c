#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "directory.h"
#include "block.h"
#include "mkfs.h"
#include "pack.h"



// helper functions from project spec
char *get_dirname(const char *path, char *dirname)
{
    strcpy(dirname, path);

    char *p = strrchr(dirname, '/');

    if (p == NULL) {
        strcpy(dirname, ".");
        return dirname;
    }

    if (p == dirname)  // Last slash is the root /
        *(p+1) = '\0';

    else
        *p = '\0';  // Last slash is not the root /

    return dirname;
}

char *get_basename(const char *path, char *basename)
{
    if (strcmp(path, "/") == 0) {
        strcpy(basename, path);
        return basename;
    }

    const char *p = strrchr(path, '/');

    if (p == NULL)
        p = path;
    else
        p++;       

    strcpy(basename, p);

    return basename;
}

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

    // fixed to increment offset
    dir->offset = offset + FIXED_LENGTH_RECORD_SIZE;

    return 0;
}

// closing the directory
void directory_close(struct directory *d)
{
    iput(d->inode);
    free(d);
}

// helper function to check if valid path
int invalid_path(char *path)
{
    char last_char = path[strlen(path) - 1];
    char first_char = path[0];
    if (strcmp(path, "/") == 0) {
        return 1;
    } else if (last_char == '/' || first_char != '/') {
        return 1;
    } else {
        return 0;
    }
}

int directory_make(char *path)
{
    // use helper function to check if path valid
    if (invalid_path(path)) {
        return -1;
    }
    // find the new directory name from the path with
    // helper functions provided
    char directory_path[1024];
    char directory_name[1024];
    get_dirname(path, directory_path);
    get_basename(path, directory_name);
    // create the new inode for new directory
    struct inode *new_directory_inode = ialloc();
    if (new_directory_inode == NULL) {
        return -1;
    }
    // create a new block-size array for new directory block
    int directory_block = alloc();
    if (directory_block == -1) {
        return -1;
    }
    struct inode *parent_inode = namei(directory_path);

    // make a block to store the directory information
	unsigned char block[BLOCK_SIZE];

    // write . file to the block
	write_u16(block, new_directory_inode->inode_num);
	strcpy((char *)block + 2, ".");
    // write .. file to the block
	write_u16(block + FIXED_LENGTH_RECORD_SIZE, parent_inode->inode_num);
	strcpy((char *)block + 2 + FIXED_LENGTH_RECORD_SIZE, "..");

    // initialize root inode
	new_directory_inode->flags = DIRECTORY_FLAG;
	new_directory_inode->size = 64;
	new_directory_inode->block_ptr[0] = get_block_position(directory_block);
    // write new directory data block to disk bwrite()
	bwrite(directory_block, block);

    // Find the block that will contain the new directory entry
    short parent_dir_data_block_num = parent_inode->block_ptr[0] / BLOCK_SIZE;
    bread(parent_dir_data_block_num, block);
    void *directory_data_location = block + parent_inode->size;

    // Write the inode number to the directory data block
    write_u16(directory_data_location, new_directory_inode->inode_num);

    // Copy the directory name to the data block
    strcpy((char *)directory_data_location + 2, directory_name);

    // Write the block to disk
    bwrite(parent_dir_data_block_num, block);

    // Update the parent directories size
    parent_inode->size += FIXED_LENGTH_RECORD_SIZE;

    // Free up the inodes
    iput(new_directory_inode);
    iput(parent_inode);

    return 0;
}
