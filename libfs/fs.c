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

struct superblock *sb;

struct fat {
	uint16_t index[BLOCK_SIZE / 2];
}__attribute__((packed));

struct fat *fat_array; //Will hold fat blocks

struct root_dir_entry {
	char filename[16];
	uint32_t size;
	uint16_t index;
	uint8_t padding[10];
}__attribute__((packed));

struct root_dir {
	struct root_dir_entry entries[128];
}__attribute__((packed));

struct root_dir *rd;
//struct root_dir_entry root_dir[ROOT_ENTRIES];

int used_fat_entries;
int used_root_entries;

int fs_open_count; // Track how many files are open

struct file_descripter {
	int root_entry; 
	size_t offset;
	const char* filename;
};

struct file_descripter *files[FS_FILE_MAX_COUNT];

// Helper functions

// Returns index of free data block
int get_free_data_index() {

	int block, index;
	int stop_flag = 0;

	for (block = 0; block < sb->fat_blk_count; block++) {
		for (index = 0; index < BLOCK_SIZE / 2; index++) {
			//printf("%i\n", fat_array[block].index[index]);
			if (!fat_array[block].index[index]) {
				stop_flag = 1;
				break;
			}
		}
		if (stop_flag) {
			break;
		}
	}

	return index + (block * BLOCK_SIZE / 2);
}

void print() {
	for (int i = 0; i < sb->fat_blk_count; i++) {
		for (int j= 0; j < BLOCK_SIZE / 2; j++) {
			printf("%i\n", fat_array[i].index[j]);
		}
	}
}

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
		//printf("Signatures do not match\n");
		return -1;
	}
	//printf("Signatures do match!\n");

	used_fat_entries = 0;
	
	//Read through all fat blocks to determine how many are used
	// THIS BREAKS STUFF BUT IDK WHY
	/*
	for (int i = 0; i < sb->fat_blk_count; i++) {
		block_read(i+1, &fat_array[i]);
		for (int j = 0; j < BLOCK_SIZE / 2; j++) {

			//If the entry is 0 that means it is empty
			if (fat_array[i].index[j]) {
				used_fat_entries++;
			}
		}
	}*/

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

	//Initialize files, dont allocate memory to the file?
	//files = malloc(FS_FILE_MAX_COUNT * sizeof(struct file_descripter));

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		files[i] = NULL;
	}

	//printf("\n");
	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */

	//TODO: SAVE EVERYTHING BACK TO THE DISK.FS

	block_disk_close();

	//Free everything that was malloced
	free(rd);
	free(fat_array);
	free(sb);
	
	//free(files);
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		free(files[i]);
	}

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
	//printf("fat_free_ratio=%d/%d\n", used_fat_entries, sb->fat_blk_count * (BLOCK_SIZE / 2)); 
	printf("fat_free_ratio=%d/%d\n", sb->data_blk_count-used_fat_entries, sb->data_blk_count);
	printf("rdir_free_ratio=%d/%d\n", ROOT_ENTRIES - used_root_entries, ROOT_ENTRIES);

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

	struct root_dir_entry *file = &rd->entries[free_index];

	// Copy filename into entry's filename
	strcpy((*file).filename, filename);
	(*file).size = 0;
	(*file).index = FAT_EOC;

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
			break;
		}
	}

	// Return -1 if file not found
	if (found_index == -1) {
		return -1;
	}

	rd->entries[found_index].filename[0] = '\0';

	// Remove data from FAT array
	while (rd->entries[found_index].index != FAT_EOC) {
		//printf("%i\n", rd->entries[found_index].index);
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

		// FAT entries start after 0 since fat_array[0].index[0] = FAT_EOC
		// Chooses the block to delete from
		int fat_block = (rd->entries[found_index].index - sb->data_blk) / (BLOCK_SIZE / 2); 
		// Chooses the index of the block to delete from
		int fat_index = (rd->entries[found_index].index - sb->data_blk) % (BLOCK_SIZE / 2); 

		// Set next index to delete
		rd->entries[found_index].index = fat_array[fat_block].index[fat_index];

		fat_array[fat_block].index[fat_index] = 0;
	}

	used_root_entries--;

	// Writing to block
	block_write(0, sb);
	for (int i = 0; i < sb->fat_blk_count; i++) {
		block_write(i + 1, &fat_array[i]);
	}
	block_write(sb->rdir_blk, rd);

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

	// Return -1 if no FS is mounted
	if (block_disk_count() == -1) {
		return -1;
	}

	//Return if max fd's has been reached
	if (fs_open_count == FS_FILE_MAX_COUNT) {
		printf("Too many files open\n");
		return -1;
	}

	//Check for null pointer
	if (!filename) {
		printf("Invalid filename\n");
		return -1;
	}
	
	//Go through open files and find empty spot
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (files[i] == NULL) {

			//Allocate memory
			files[i] = (struct file_descripter*)malloc(sizeof(struct file_descripter));
			if (files[i] == NULL) {
				printf("file descripter failed to allocate enough memory\n");
				return -1;
			}

			//Check for file in root entry
			files[i]->root_entry = -1;

			for (int j = 0; j < ROOT_ENTRIES; j++) {
				if (!strcmp(rd->entries[j].filename, filename)) {
					files[i]->root_entry = j;
					break;
				}
			}

			// Return -1 if file not found
			if (files[i]->root_entry == -1) {
				printf("File not found\n");
				return -1;
			}

			//Set defaults
			files[i]->filename = filename;
			files[i]->offset = 0;

			fs_open_count++;

			//Return fd
			return i;
		}
	}

	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */

	// Return -1 if no FS is mounted
	if (block_disk_count() == -1) {
		return -1;
	}

	//Check for bounds error
	if (fd > FS_FILE_MAX_COUNT - 1 || fd < 0) {
		printf("file out of bounds\n");
		return -1;
	}

	//Check to see if the fd exists
	if (files[fd] == NULL) {
		printf("File descripter not found\n");
		return -1;
	}

	//Remove file descripter object
	free(files[fd]);
	files[fd] = NULL;

	fs_open_count--;

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */

	// Return -1 if no FS is mounted
	if (block_disk_count() == -1) {
		return -1;
	}

	//Check for bounds error
	if (fd > FS_FILE_MAX_COUNT - 1 || fd < 0) {
		printf("file out of bounds\n");
		return -1;
	}

	//Check to see if the fd exists
	if (files[fd] == NULL) {
		printf("File descripter not found\n");
		return -1;
	}

	//Find size from the file descripter which the fd knows its entry
	return rd->entries[files[fd]->root_entry].size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */

	// Return -1 if no FS is mounted
	if (block_disk_count() == -1) {
		return -1;
	}

	//Check for bounds error
	if (fd > FS_FILE_MAX_COUNT - 1 || fd < 0) {
		printf("file out of bounds\n");
		return -1;
	}

	//Check to see if the fd exists
	if (files[fd] == NULL) {
		printf("File descripter not found\n");
		return -1;
	}

	//Check if the offset is greater than the size of the file
	if (offset > rd->entries[files[fd]->root_entry].size) {
		printf("Offset is bigger than file size\n");
		return -1;
	}

	files[fd]->offset = offset;

	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	// Return -1 if no FS is mounted or fd is out of bounds or fd isn't open or buffer doesn't exist
	if (block_disk_count() == -1 || fd > FS_FILE_MAX_COUNT - 1 || fd < 0 || !files[fd] || !buf) {
		return -1;
	}

	// No need to do anything if count is less than 1
	if (count < 1) {
		return 0;
	}

	struct root_dir_entry *file = &rd->entries[files[fd]->root_entry];

	// Get locations of data and FAT to update

	int free_data_index = get_free_data_index();
	int free_fat_block = free_data_index / (BLOCK_SIZE / 2);
	int free_fat_index = free_data_index % (BLOCK_SIZE / 2);

	/*
	We need to see if count + offset is greater than BLOCK_SIZE
	If it is, append part of string to block and then fill new data block and update FAT
	*/

	char bounce[count];

	strncpy(bounce, buf, count);

	block_write(sb->data_blk + free_data_index, bounce);

	// Update FAT array
	fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;
	block_write(free_fat_block + 1, &fat_array[free_fat_block]);

	// Update root directory entry's size and index of data block
	(*file).size = count; // strlen(count) maybe?
	(*file).index = sb->data_blk + free_data_index;
	block_write(sb->rdir_blk, rd);

	return strlen(buf); // Return number of bytes written
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	// Return -1 if no FS is mounted or fd is out of bounds or fd isn't open or buffer doesn't exist
	if (block_disk_count() == -1 || fd > FS_FILE_MAX_COUNT - 1 || fd < 0 || !files[fd] || !buf) {
		return -1;
	}

	// No need to do anything if count is less than 1
	if (count < 1) {
		return 0;
	}

	struct root_dir_entry *file = &rd->entries[files[fd]->root_entry];

	char bounce[BLOCK_SIZE * ((*file).size / BLOCK_SIZE + 1)];
	memset(bounce, '\0', BLOCK_SIZE * ((*file).size / BLOCK_SIZE + 1));
	char block_bounce[BLOCK_SIZE];

	block_read((*file).index, block_bounce);
	printf("bb: %s\n", block_bounce);
	strcat(bounce, block_bounce);
	printf("bb: %s\n", bounce);

	strncpy(buf, bounce + files[fd]->offset, count);

	/*
	struct root_dir_entry *file = &rd->entries[files[fd]->root_entry];

	// Find blocks file takes up
	int total_blocks = (*file).size / BLOCK_SIZE + 1;

	// bounce holds final buffer, block_bounce holds block specific buffer
	char bounce[total_blocks * BLOCK_SIZE];
	char block_bounce[BLOCK_SIZE];

	// Gets index of data array
	int next_index = (*file).index;
	int fat_block = (next_index - sb->data_blk) / (BLOCK_SIZE / 2);
	int fat_index = (next_index - sb->data_blk) % (BLOCK_SIZE / 2);
	int fat_entry = fat_array[fat_block].index[fat_index];

	// Continue concatenating string until reaching FAT_EOC
	while (1) {
		block_read(next_index, block_bounce);
		strcat(bounce, block_bounce);

		if (fat_entry == FAT_EOC) {
			break;
		}

		fat_block = fat_entry / (BLOCK_SIZE / 2);
		fat_index = fat_entry % (BLOCK_SIZE / 2);

		fat_entry = fat_array[fat_block].index[fat_index];
		next_index = fat_entry + sb->data_blk;
	}

	strncpy(buf, bounce + files[fd]->offset, count);
	*/

	return 0;
}

