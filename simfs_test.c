#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ctest.h"
#include "image.h"
#include "block.h"
#include "free.h"
#include "inode.h"
#include "mkfs.h"
#include "pack.h"
#include "directory.h"
#include "ls.h"

// macros
#define FREE_BLOCK_MAP_NUM 2
#define BLOCK_SIZE 4096
#define ONLY_ONE 255
#define NEW_INODE_NUM 256

void block_for_testing(unsigned char *block, int value) {
	for (int i = 0; i < BLOCK_SIZE; i++) {
		block[i] = value;
	}
}

#ifdef CTEST_ENABLE

void test_bread_and_bwrite(void)
{
	unsigned char test_array[BLOCK_SIZE] = {0};
    
	int block_num = 1;
	image_open("test_image", 0);
	bwrite(block_num, test_array);

	unsigned char read_block[BLOCK_SIZE];

	bread(block_num, read_block);
	CTEST_ASSERT(memcmp(test_array, read_block, BLOCK_SIZE) == 0, "testing reading and writing from the same block");

	image_close();
}

void test_image_open_and_close(void)
{
	int image_fd = image_open("test_image", 0);
	CTEST_ASSERT(image_fd == 3, "testing creating and opening image file");
	CTEST_ASSERT(image_close() == 0, "testing closing image file");

	image_fd = image_open("test_image", 1);
	CTEST_ASSERT(image_fd == 3, "testing creation and truncation of new image");
	CTEST_ASSERT(image_close() == 0, "testing closing file");
}

void test_set_free(void)
{
    // arbitrary value for testing
	int num = 420;

    // computations from project spec
    int byte_num = num / BYTE;
	int bit_num = num % BYTE;

	unsigned char test_array[BLOCK_SIZE] = {0};
	set_free(test_array, num, 1); 
	unsigned char character = test_array[byte_num];
	int bit = (character >> bit_num) & 1;

	CTEST_ASSERT(bit == 1, "testing if in use");

	// set a free bit and get it from character
	set_free(test_array, num, 0); 
	character = test_array[byte_num];
	bit = (character >> bit_num) & 0;	

	CTEST_ASSERT(bit == 0, "testing if free");
}

void test_find_free(void)
{
    // arbitrary value for test
	int num = 69; 
	unsigned char test_block[BLOCK_SIZE];
	block_for_testing(test_block, ONLY_ONE);
	
	// make bit free and check for the bit
	set_free(test_block, num, 0);
	CTEST_ASSERT(find_free(test_block) == num, "testing if find_free locates free block");
}

void test_alloc(void)
{
    // arbitrary value for testing
	int num = 69;
	image_open("test_image", 0);

	unsigned char test_block[BLOCK_SIZE];
	block_for_testing(test_block, ONLY_ONE);
	bwrite(FREE_BLOCK_MAP_NUM, test_block);
	
    // allocate
	int alloc_num = alloc();
	CTEST_ASSERT(alloc_num == -1, "testing with no free blocks");

	block_for_testing(test_block, ONLY_ONE);
	set_free(test_block, num, 0);
	bwrite(FREE_BLOCK_MAP_NUM, test_block);

    // allocate
	alloc_num = alloc();
	CTEST_ASSERT(alloc_num == num, "testing if alloc() finds free block");

	image_close();	
}

void test_ialloc(void)
{
    // arbitrary value for testing
	int num = 21;
	image_fd = image_open("test_image", 0);
	unsigned char test_block[BLOCK_SIZE];

    // test block set to all ones
	block_for_testing(test_block, ONLY_ONE);
	bwrite(1, test_block);

    // allocate
	struct inode *ialloc_inode = ialloc();
	CTEST_ASSERT(ialloc_inode == NULL, "testing when no free nodes in inode mp");

    // test block to all ones
	block_for_testing(test_block, ONLY_ONE);
	set_free(test_block, num, 0);

    // write to the map
	bwrite(1, test_block);
	ialloc_inode = ialloc();
	int ialloc_num = ialloc_inode->inode_num;
	CTEST_ASSERT(ialloc_num == num, "testing if ialloc finds the free inode");

	mark_incore_in_use();
	ialloc_inode = ialloc();
	CTEST_ASSERT(ialloc_inode == NULL, "testing no freeincore inodes");

	image_close();
}

void test_mkfs(void)
{
	// free block should be 8 now, 7 is root directory and 0-6 marked in use
	int expected_next_free_block = 8;
	image_open("test_image", 0);
	mkfs();
	unsigned char block[BLOCK_SIZE];
	bread(2, block);
	// check next free block
	int next_free = find_free(block);
	CTEST_ASSERT(next_free == expected_next_free_block, "test that free block should be 8");
	// read root directory
	bread(7, block);

	// unpack first values
	int inode_number = read_u16(block);
	char file_name[3];
	memcpy(file_name, block + FILE_OFFSET, 3);
	CTEST_ASSERT(inode_number == 0, "test taht root number is 0");
	CTEST_ASSERT(strcmp(file_name, ".") == 0, "first entry is '.'");
	// unpack second values
	inode_number = read_u16(block + FIXED_LENGTH_RECORD_SIZE);
	memcpy(file_name, block + FIXED_LENGTH_RECORD_SIZE + FILE_OFFSET, 3);
	CTEST_ASSERT(inode_number == 0, "test root inode number is 0");
	CTEST_ASSERT(strcmp(file_name, "..") == 0, "test that first directory entry is '..'");
	image_close();
}

void test_read_and_write_inode(void)
{
	unsigned int test_num = 420;
	image_open("test_image", 0);
	struct inode *new_inode = find_incore_free();

	// arbitrary values for testing
	new_inode->inode_num = test_num;
	new_inode->size = 420;
	new_inode->owner_id = 5;
	new_inode->permissions = 10;
	new_inode->flags = 15;
	new_inode->link_count = 20;
	new_inode->block_ptr[0] = 25;
	write_inode(new_inode);
	struct inode inode_read_buffer = {0};
	read_inode(&inode_read_buffer, test_num);
	image_close();

	// test that all values match
	CTEST_ASSERT(new_inode->size == inode_read_buffer.size, "testing if write/read size are the same");
	CTEST_ASSERT(new_inode->owner_id == inode_read_buffer.owner_id, "testing for write/read owner_id are matching");
	CTEST_ASSERT(new_inode->permissions == inode_read_buffer.permissions, "testing to see write/read permissions are matching");
	CTEST_ASSERT(new_inode->flags == inode_read_buffer.flags, "testing if write/read flags are matching");
	CTEST_ASSERT(new_inode->link_count == inode_read_buffer.link_count, "testing if write/read link_count are matching");
	CTEST_ASSERT(new_inode->block_ptr[0] == inode_read_buffer.block_ptr[0], "testing if write/read block_ptr[0] are matching");
}

void test_iget(void)
{
	image_open("test_image", 0);
	unsigned int test_num = 69;
	struct inode *available_inode = find_incore_free();

	CTEST_ASSERT(available_inode->ref_count == 0, "testing if ref_count is initially zero");

	// give arbitrary value to the inode
	available_inode->inode_num = test_num;
	struct inode *iget_inode = iget(test_num);

	CTEST_ASSERT(iget_inode->ref_count == 1, "testing if ref_count +1 after iget");
	CTEST_ASSERT(memcmp(available_inode, iget_inode, INODE_SIZE) == 0, "testing if the right inode is gotten");

	clear_incore_inodes();
	iget_inode = iget(test_num);

	CTEST_ASSERT(iget_inode != NULL, "testing if a new incore is returned");
	CTEST_ASSERT(iget_inode->inode_num == test_num, "testing if new inode has right inode_num");

	unsigned int new_inode_num = NEW_INODE_NUM;
	mark_incore_in_use();
	iget_inode = iget(new_inode_num);

	CTEST_ASSERT(iget_inode == NULL, "testing case when no free incore inode and inod number nonexistant");
	clear_incore_inodes();
	image_close();
}

void test_iput(void)
{
	image_open("test_image", 0);
	// arbitrary value for testing
	unsigned int test_num = 50;
	struct inode *fake_inode = iget(test_num);

	iput(fake_inode);
	CTEST_ASSERT(fake_inode->ref_count == 0, "testing if ref_count -1 after iput");

	struct inode *iget_inode = iget(test_num);

	CTEST_ASSERT(memcmp(fake_inode, iget_inode, INODE_SIZE) == 0, "testing iput and iget togehter");
	image_close();
}


void test_directory(void)
{
	int root_directory = 0;
	image_open("test_image", 0);
	mkfs();

	// create entry struture, then open and read directory
	struct directory_entry *ent = malloc(sizeof(struct directory_entry));
	struct directory *dir = directory_open(root_directory);
	int directory_get_return_value = directory_get(dir, ent);
	char *file_name = ent->name;
	CTEST_ASSERT(directory_get_return_value == 0, "test successful get returns 0");
	CTEST_ASSERT(strcmp(file_name, ".") == 0, "test that directory entry is '.'");
	CTEST_ASSERT(ent->inode_num == 0, "test entry inode number is 0");
	directory_close(dir);
	free(ent);

	image_close();
}

void test_directory_failures(void)
{
	int root_directory = 0;
	image_open("test_image", 0);
	mkfs();

	// cause iget to fail intentionally and ensure null is returned
	mark_incore_in_use();
	struct directory *dir = directory_open(256);
	CTEST_ASSERT(dir == NULL, "test if iget fails null is returned");
	clear_incore_inodes();
	// create directory entry struct, then open it and givge it a high value
	struct directory_entry *ent = malloc(sizeof(struct directory_entry));
	dir = directory_open(root_directory);
	dir->offset = 123456789;
	int directory_get_return_value = directory_get(dir, ent);
	// ensure -1 is returned
	CTEST_ASSERT(directory_get_return_value == -1, "test that a failed directory returned -1");
	directory_close(dir);
	free(ent);

	image_close();
}

void test_ls(void)
{
	image_open("test_image", 0);
	mkfs();
	printf("\nls\n");
	ls(0);
	image_close();
}

void test_namei(void)
{
	unsigned int root_inode = 0;
	image_open("test_image", 0);
	mkfs();
	// namei returns root node
	struct inode *in = namei("/");
	CTEST_ASSERT(in->inode_num == root_inode, "test namei returns root inode");
	image_close();
}

void test_directory_make_failures(void)
{
	image_open("test_image", 0);
	mkfs();
	int directory_make_status = directory_make("/foo/");
	CTEST_ASSERT(directory_make_status == -1, "testing directory path invalid names");
	directory_make_status = directory_make("foo");
	CTEST_ASSERT(directory_make_status == -1, "testing directory path invalid names");
	// block map as a backup
	unsigned char original_state[BLOCK_SIZE];
	bread(FREE_BLOCK_MAP_NUM, original_state);
	unsigned char test_block[BLOCK_SIZE];
	memset(test_block, ONLY_ONE, BLOCK_SIZE);

	bwrite(FREE_BLOCK_MAP_NUM, test_block);
	directory_make_status = directory_make("/baz");
	CTEST_ASSERT(directory_make_status == -1, "testing no data blocks should lead to failure");
	bwrite(FREE_BLOCK_MAP_NUM, original_state);

	bread(1, original_state);
	bwrite(1, test_block);
	directory_make_status = directory_make("/baz");
	CTEST_ASSERT(directory_make_status == -1, "testing no inode blocks lead to failure");

	image_close();
}

void test_directory_make_success(void)
{
	unsigned int test_inode_num = 1;
	image_open("test_image", 0);
	mkfs();
	int directory_creation_status = directory_make("/foo");
	CTEST_ASSERT(directory_creation_status == 0, "testing directory creation returns 0 on success");
	
	struct directory *dir = directory_open(test_inode_num);
	struct directory_entry *ent = malloc(sizeof(struct directory_entry));
	directory_get(dir, ent);
	CTEST_ASSERT(strcmp(ent->name, ".") == 0, "testing return 0");
	CTEST_ASSERT(ent->inode_num == test_inode_num, "testing retrieved directory entry is the same as inode number from directory make");

	directory_make("/foo/bar");
	image_close();
}

int main(void)
{
    CTEST_VERBOSE(1);
	test_image_open_and_close();
	test_bread_and_bwrite();
	test_set_free();
	test_find_free();
	test_alloc();
	test_ialloc();
	test_mkfs();
	// test_read_and_write_inode();
	// test_iget();
	// test_iput();
	test_directory();
	test_directory_failures();
	test_namei();
	test_directory_make_failures();
	test_directory_make_success();
	test_ls();

    CTEST_RESULTS();
    CTEST_COLOR(1);
    CTEST_EXIT();
}

#endif