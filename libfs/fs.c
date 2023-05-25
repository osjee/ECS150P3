#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

typedef __uint128_t uint128_t;
#define ROOT_ENTRIES 128

/* TODO: Phase 1 */
struct superblock {
	uint64_t signature;
	uint16_t total_blk_count;
	uint16_t rdir_blk;
	uint16_t data_blk;
	uint16_t data_blk_count;
	uint8_t fat_blk_count;
	uint8_t padding[4079];
}__attribute__((packed));

struct fat {
	uint16_t index[BLOCK_SIZE / 2];
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

struct fat *fat_array; //Will hold fat blocks

struct superblock *sb;

struct root_dir *rt;

int used_fat_entries;
int used_root_entries;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	block_disk_open(diskname);

	//Make sure to check data before initializing superblock

	//Allocate memory for superblock
	sb = (struct superblock*)malloc(sizeof(struct superblock));

	if (sb == NULL) {
		printf("Superblock failed to allocate");
		return -1;
	}

	//Allocate memory for root_dir
	rt = (struct root_dir*)malloc(sizeof(struct root_dir));

	if (rt == NULL) {
		printf("root failed to allocate");
		return -1;
	}

	//Variable to hold the block you read from
	//uint8_t block[BLOCK_SIZE]; //Might use later

	//Read the super block from virtual disk
	if (block_read(0, sb)) {
		printf("read failed");
	}

	//Allocate memory for fat_array (after reading superblock, we now know how many fat blocks there are)
	fat_array = (struct fat*)malloc(sb->fat_blk_count * sizeof(struct fat));

	if (fat_array == NULL) {
		printf("Fat array failed to allocate");
		return -1;
	}

	//Setup signature to compare to
	char* default_sig = "ECS150FS";

	//Check if block has correct signature
	if (memcmp(&sb->signature, default_sig, sizeof(uint64_t))) {
		printf("Signatures do not match\n");
		return -1;
	}
	printf("Signatures do match!\n");

	used_fat_entries = 0;
	
	//Read through all fat blocks to determine how many are used
	for (int i = 0; i < sb->fat_blk_count; i++) {
		block_read(i+1, &fat_array[i]);
		for (int j = 0; j < BLOCK_SIZE / 2; j++) {

			//If the entry is 0 that means it is empty
			if (fat_array[i].index[j] == 0) {
				used_fat_entries += 1;
			}
		}
	}

	used_root_entries = 0;

	//Needed to compare the filename of the root entry
	char* null_char = "\0";

	//Go through the root block and count used entries
	block_read(sb->rdir_blk, rt);
	for (int j = 0; j < ROOT_ENTRIES; j++) {

		//If the filename is \0 that means the entry is empty
		if (memcmp(&rt->entries[j].filename, null_char, sizeof(uint128_t))) {
			used_root_entries += 1;
		}
	}

	printf("\n");
	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */

	block_disk_close();

	//Free everything that was malloced
	free(rt);
	free(fat_array);
	free(sb);

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb->total_blk_count);
	printf("fat_blk_count=%d\n", sb->fat_blk_count);
	printf("rdir_blk=%d\n", sb->rdir_blk); //sb->fat_blk_count + 1
	printf("data_blk=%d\n", sb->data_blk); //sb->fat_blk_count + 2
	printf("data_blk_count=%d\n", sb->data_blk_count);
	printf("fat_free_ratio=%d/%d\n", used_fat_entries, sb->fat_blk_count*(BLOCK_SIZE / 2));
	printf("rdir_free_ratio=%d/%d\n", used_root_entries, ROOT_ENTRIES);

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

