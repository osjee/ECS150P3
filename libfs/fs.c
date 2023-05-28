#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

typedef __uint128_t uint128_t;
#define ROOT_ENTRIES 128
#define FAT_EOC 0xFFFF

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
	char filename[16];
	uint32_t size;
	uint16_t index;
	uint8_t padding[10];
}__attribute__((packed));


struct root_dir {
	struct root_dir_entry entries[128];
}__attribute__((packed));


struct fat *fat_array; //Will hold fat blocks

struct superblock *sb;

struct root_dir *rd;
//struct root_dir_entry root_dir[ROOT_ENTRIES];

int used_fat_entries;
int used_root_entries;

int fs_open_count; // Track how many files are open

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
	rd = (struct root_dir*)malloc(sizeof(struct root_dir));

	if (rd == NULL) {
		printf("Root failed to allocate");
		return -1;
	}

	//Variable to hold the block you read from
	//uint8_t block[BLOCK_SIZE]; //Might use later

	//Read the super block from virtual disk
	if (block_read(0, sb)) {
		printf("Read failed");
	}

	//Allocate memory for fat_array (after reading superblock, we now know how many fat blocks there are)
	fat_array = (struct fat*)malloc(sb->fat_blk_count * sizeof(struct fat));

	if (fat_array == NULL) {
		printf("Fat array failed to allocate");
		return -1;
	}

	// First entry in FAT array should be FAT_EOC
	fat_array[0].index[0] = FAT_EOC;

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
				used_fat_entries++;
			}
		}
	}

	used_root_entries = 0;

	//Go through the root block and count used entries
	block_read(sb->rdir_blk, rd);
	for (int i = 0; i < ROOT_ENTRIES; i++) {

		//If the filename is \0 that means the entry is empty
		/*
		if (memcmp(&rd->entries[j].filename, null_char, sizeof(uint128_t))) {
			used_root_entries++;
		}
		*/
		if (rd->entries[i].filename[0]) {
			used_root_entries++;
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
	free(rd);
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
	printf("fat_free_ratio=%d/%d\n", used_fat_entries, sb->fat_blk_count * (BLOCK_SIZE / 2));
	printf("rdir_free_ratio=%d/%d\n", used_root_entries, ROOT_ENTRIES);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */

	// Return -1 if no FS is mounted or filename is invalid or filename length is too long or root directory contains maximum file count
	if (block_disk_count() == -1 || !filename || sizeof(filename) > sizeof(uint128_t) || used_root_entries == 128) { // Change to 128 later (need to change data type of filename maybe)
		return -1;
	}

	int free_index = -1;

	for (int i = 0; i < ROOT_ENTRIES; i++) {
		// Return -1 if filename exists
		if (rd->entries[i].filename == filename) {
			return -1;
		}

		// Get first free index
		if (rd->entries[i].filename[0] == '\0' && free_index == -1) {
			free_index = i;
		}
	}

	// Copy filename into entry's filename
	strcpy(rd->entries[free_index].filename, filename);
	rd->entries[free_index].size = 0;
	rd->entries[free_index].index = FAT_EOC;

	used_root_entries++;

	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	// Return -1 if no FS is mounted or filename is invalid
	if (block_disk_count() == -1 || !filename) { // CHECK IF FILE IS CURRENTLY OPEN
		return -1;
	}

	int found_index = -1;

	for (int i = 0; i < ROOT_ENTRIES; i++) {
		if (!strcmp(rd->entries[i].filename, filename)) {
			found_index = i;
		}
	}

	// Return -1 if file not found
	if (found_index == -1) {
		return -1;
	}

	rd->entries[found_index].filename[0] = '\0';

	// Remove data from FAT array
	while (rd->entries[found_index].index != FAT_EOC) {
		/*
		Block to delete from is calculated by the index / by the number of entries in struct fat (2048)
		Index to delete from is calculated by the index - (fat block to delete * by the number of entries in struct fat (2048))

		Example using index 1:
		Block to delete from: 1 / 2048 = 0
		Index to delete from: 1 - (0 * 2048) = 1

		Example using index 2048:
		Block to delete from: 2048 / 2048 = 1
		Index to delete from: 2048 - (1 * 2048) = 0
		*/

		// Chooses the block to delete from
		int fat_block = rd->entries[found_index].index / (BLOCK_SIZE / 2); 
		// Chooses the index of the block to delete from
		int fat_index = rd->entries[found_index].index - fat_block * (BLOCK_SIZE / 2); 

		// Set next index to delete
		rd->entries[found_index].index = fat_array[fat_block].index[fat_index];

		fat_array[fat_block].index[fat_index] = 0;
	}

	used_root_entries--;

	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */

	printf("FS Ls:\n");

	// Return -1 if no FS is mounted
	if (block_disk_count() == -1) {
		return -1;
	}

	for (int i = 0; i < ROOT_ENTRIES; i++) {
		if (rd->entries[i].filename[0]) {
			printf("file: %s, size: %i, data_blk: %i\n", rd->entries[i].filename, rd->entries[i].size, rd->entries[i].index);
		}
	}

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

