#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "image.h"
#include "block.h"
#include "mkfs.h"
#include "inode.h"
#include "pack.h"
#include "directory.h"

// construct the file system
// 1. zero out every block of the file system.
// 2. mark blocks 0-6 as allocated in the free block map.
// 3. add the root directory and other things to bootstrap the 
// file system

// create the file system
void mkfs(void)
{
	unsigned char initialize_data[FOUR_MB_IMAGE];
	memset(initialize_data, 0, FOUR_MB_IMAGE);
	write(image_fd, initialize_data, FOUR_MB_IMAGE);
	for (int i = 0; i < METADATA; i++) {
		alloc();
	}
    // call ialloc to get a new inode
	struct inode *root_inode = ialloc();
    // call alloc to get a new data block
	int directory_block = alloc();
	// initiaize the inode returned from ialloc.
    // flags set to 2, size set to bye size of directory (64)
	root_inode->flags = DIRECTORY_FLAG;
	root_inode->size = ROOT_DIR_SIZE;
	root_inode->block_ptr[0] = get_block_position(directory_block);
    // make this array to populate with new directory data
	unsigned char block[BLOCK_SIZE];

	// pack the . and .. directory entries in here
	write_u16(block, root_inode->inode_num);
	strcpy((char *)block + FILE_OFFSET, ".");
	write_u16(block + FIXED_LENGTH_RECORD_SIZE, root_inode->inode_num);
	strcpy((char *)block + FILE_OFFSET + FIXED_LENGTH_RECORD_SIZE, "..");
	// write the directory data block back out to disk with bwrite()
	bwrite(directory_block, block);
	// write new directory inode out to disk and free incore inode
	iput(root_inode);
}