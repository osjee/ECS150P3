#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

typedef __uint128_t uint128_t;

/* TODO: Phase 1 */
struct superblock {
	uint64_t signature;
	uint16_t total_blocks;
	uint16_t root_index;
	uint16_t data_start_index;
	uint16_t num_data_blocks;
	uint8_t num_fat_blocks;
	uint8_t padding[4079];
}__attribute__((packed));

struct fat {
	uint16_t index[BLOCK_SIZE / 8];
}__attribute__((packed));

struct root_dir_entry {
	uint128_t filename;
	uint32_t size;
	uint16_t index;
	uint8_t padding[10];
}__attribute__((packed));

struct root_dir {
	struct root_dir_entry entries[128];
}__attribute__((packed));

uint16_t fat_array;

struct superblock *sb;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	block_disk_open(diskname);

	//Make sure to check data before initializing superblock

	//Allocate memory for superblock
	struct superblock* sb = (struct superblock*)malloc(sizeof(struct superblock));

	//Variable to hold the block you read from
	//uint8_t block[BLOCK_SIZE]; Might user later

	//Read the super block from virtual disk
	if (block_read(0, sb)) {
		printf("read failed");
	}

	//Setup signature to compare to
	char* defaultSig = "ECS150FS";

	//Check if block has correct signature
	if (memcmp(&sb->signature,defaultSig,sizeof(uint64_t))) {
		printf("Signatures do not match\n");
		return -1;
	}
	printf("Signatures do match!\n");

	/*

	sb->signature = (uint64_t)"ECS150FS";

	uint16_t num_data_blocks = block_disk_count();
	uint8_t num_fat_blocks = num_data_blocks * 2 / BLOCK_SIZE;
	uint16_t root_index = num_fat_blocks + 1;
	uint16_t data_start_index = root_index + 1;
	uint16_t total_blocks = 2 + num_data_blocks + num_fat_blocks;

	sb->total_blocks = total_blocks;
	sb->root_index = root_index;
	sb->data_start_index = data_start_index;
	sb->num_data_blocks = num_data_blocks;
	sb->num_fat_blocks = num_fat_blocks;

	*/

	//block_write(0, sb);

	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */

	block_disk_close();

	free(sb);

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb->total_blocks);
	printf("fat_blk_count=%d\n", sb->num_fat_blocks);
	//printf("rdir_blk=%d\n", sb->); not from superblock
	//printf("data_blk=%d\n", sb->); not from superblock
	printf("data_blk_count=%d\n", sb->num_data_blocks);
	//printf("fat_free_ratio=%d\n", sb->); not from superblock
	//printf("rdir_free_ratio%d\n", sb->); not from superblock

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */

	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */

	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */

	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */

	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	return 0;
}

