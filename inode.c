#include "block.h"
#include "free.h"
#include "inode.h"
#include "pack.h"
#include "pack.c"
#include <string.h>
#include <stdio.h>

static struct inode incore[MAX_SYS_OPEN_FILES] = {0};

// find the first free in-core inode within the incore array
struct inode *find_incore_free(void){
	for (int i = 0; i < MAX_SYS_OPEN_FILES; i++) {
		// check if reference is 0
		struct inode *temp_incore = &incore[i];

		if (temp_incore->ref_count == 0) {
			return temp_incore;
		}
	}
	return NULL;
}

// find an incore inode record in the incore array
// by the inode number
struct inode *find_incore(unsigned int inode_num){
	for (int i = 0; i < MAX_SYS_OPEN_FILES; i++) {
		// check if reference is 0
		struct inode *temp_incore = &incore[i];

		if(temp_incore->inode_num == inode_num){
			if(temp_incore->ref_count != 0){
				return temp_incore;
			}
		}
	}
	return NULL;
}

// take a pointer to an empty struct inode to read
// data into.
void read_inode(struct inode *in, int inode_num){
	// helper code from project spec
	int block_num = inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;
	int block_offset = inode_num % INODES_PER_BLOCK;
	int block_offset_bytes = block_offset * INODE_SIZE;
	unsigned char read_buffer[BLOCK_SIZE];
	bread(block_num, read_buffer);

	// layout of data as stored on disk for each record
	// read using functions from pack.c
    in->size = read_u32(read_buffer + block_offset_bytes);
    in->owner_id = read_u16(read_buffer + block_offset_bytes + OWNER_ID_OFFSET);
    in->permissions = read_u8(read_buffer + block_offset_bytes + PERMISSIONS_OFFSET);
    in->flags = read_u8(read_buffer + block_offset_bytes + FLAGS_OFFSET);
    in->link_count = read_u8(read_buffer + block_offset_bytes + LINK_COUNT_OFFSET);

    // read pointers to inode using functions from pack.c
    int block_pointer_address = BLOCK_POINTER_OFFSET;
    for (int i = 0; i < INODE_PTR_COUNT; i++) {
    	in->block_ptr[i] = read_u16(read_buffer + block_offset_bytes + block_pointer_address);
    	block_pointer_address += 2;
    }	
}

// stores inode data pointed to by in on disk
void write_inode(struct inode *in){
	int inode_num = in->inode_num;
	// helper code from project spec
	int block_num = inode_num / INODES_PER_BLOCK + INODE_FIRST_BLOCK;
	int block_offset = inode_num % INODES_PER_BLOCK;
	int block_offset_bytes = block_offset * INODE_SIZE;
	unsigned char write_buffer[BLOCK_SIZE];

	// layout of data as stored on disk for each record
	// write functions from pack.c
 	write_u32(write_buffer + block_offset_bytes, in->size);
    write_u16(write_buffer + block_offset_bytes + OWNER_ID_OFFSET, in->owner_id);
    write_u8(write_buffer + block_offset_bytes + PERMISSIONS_OFFSET, in->permissions);
    write_u8(write_buffer + block_offset_bytes + FLAGS_OFFSET, in->flags);
    write_u8(write_buffer + block_offset_bytes + LINK_COUNT_OFFSET, in->link_count);
    // write pointers to inode using functions from pack.c
    int block_pointer_address = BLOCK_POINTER_OFFSET;
    for (int i = 0; i < INODE_PTR_COUNT; i++) {
    	write_u16(write_buffer + block_offset_bytes + block_pointer_address, in->block_ptr[i]);
    	block_pointer_address += 2;
    }
    // write to disk
	bwrite(block_num, write_buffer);
}

void clear_incore_inodes(void)
{
	for (int i = 0; i < MAX_SYS_OPEN_FILES; i++) {
		incore[i].ref_count = 0;
	}
}

void mark_incore_in_use(void)
{
	for (int i = 0; i < MAX_SYS_OPEN_FILES; i++) {
		incore[i].ref_count = 1;
	}
}

// iget function to return a pointer to an incore inode
// for a given inode number, following project spec algorithm
struct inode *iget(int inode_num){
	// use find_incore() to search for inode number incore
	struct inode *incore_inode = find_incore(inode_num);
	// if found
	if (incore_inode != NULL) {
		// increment the ref count and return the pointer
		incore_inode->ref_count++;
		return incore_inode;
	} else {
		// else, find a free incore inode
		struct inode *available_incore = find_incore_free();
		// if none found, return null
		if (available_incore == NULL) {
			return NULL;
		}
		// read the data from disk into read_inode()
		read_inode(available_incore, inode_num);
		// set inode ref_count to 1
		available_incore->ref_count = 1;
		// set inode's inode_num to inode num that was passed in
		available_incore->inode_num = inode_num;
		// return the pointer to the inode
		return available_incore;
	}
}

// opposite of iget(), frees the node if no one is using it
void iput(struct inode *in){
	// if ref_count on in is already 0, return
	if (in->ref_count == 0) {
		return;
	} else {
		// decrement ref count
		in->ref_count--;
		// if ref_count is 0
		if(in->ref_count == 0) {
			// save the inode to disk with write_inode()
			write_inode(in);
		}
	}
}

// helper function to create a new incor einode
void new_incore_inode(struct inode *in, int inode_num)
{
	// initialize all values at 0
	in->size = 0;
	in->owner_id = 0;
	in->permissions = 0;
	in->flags = 0;
    for (int i = 0; i < INODE_PTR_COUNT; i++) {
    	in->block_ptr[i] = 0;
    }
    // give it an inode number
    in->inode_num = inode_num;
}

// allocate blocks from theri respective free maps
// expanded for project 6
struct inode *ialloc(void){
	unsigned char block[BLOCK_SIZE] = {0};
    // get the inode map
	bread(1, block);
    // locate a free inode
	int num = find_free(block);

    // if there are no free inodes, return null
	if (num == FAILED) {
		return NULL;
	} else {
        // mark it as non free
		set_free(block, num, 1);
        // save inode back out to disk
		bwrite(1, block);
	    // Get an in-core version of the inode (iget())
		struct inode *incore_inode = iget(num);
	    // If not found:
	    if (incore_inode == NULL) {
	    	return NULL;
	    } else {
		    // Initialize the inode:
	    	new_incore_inode(incore_inode, num);
		    // Save the inode to disk (write_inode())
	    	write_inode(incore_inode);
		    // Return the pointer to the in-core inode.
	    	return incore_inode;
	    }
	}
}