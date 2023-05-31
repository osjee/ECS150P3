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
int get_free_data_block() {

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

	if (!stop_flag) {
		return -1;
	}

	return index + (block * BLOCK_SIZE / 2);
}

// Returns entry of FAT from data block
int get_next_data_block(int db) {
	int fat_block = db / (BLOCK_SIZE / 2);
	int fat_index = db % (BLOCK_SIZE / 2);
	return fat_array[fat_block].index[fat_index];
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
	//fat_array[0].index[0] = FAT_EOC; done already? probably lol

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
	for (int i = 0; i < sb->fat_blk_count; i++) {
		block_read(i+1, &fat_array[i]);
		for (int j = 0; j < BLOCK_SIZE / 2; j++) {

			//If the entry is 0 that means it is empty
			if (fat_array[i].index[j]) {
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

	// Writing to block
	block_write(0, sb);
	for (int i = 0; i < sb->fat_blk_count; i++) {
		block_write(i + 1, &fat_array[i]);
	}
	block_write(sb->rdir_blk, rd);

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
		if (!strcmp(rd->entries[i].filename, filename)) {
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
	int to_delete = rd->entries[found_index].index;

	// Remove data from FAT array
	while (1) {
		// Chooses the block to delete from
		int fat_block = to_delete / (BLOCK_SIZE / 2); 
		// Chooses the index of the block to delete from
		int fat_index = to_delete % (BLOCK_SIZE / 2); 

		if (get_next_data_block(to_delete) == FAT_EOC) {
			fat_array[fat_block].index[fat_index] = 0;
			break;
		}

		to_delete = get_next_data_block(to_delete);

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


	/*
	if new size (in blocks) is greater than old size, get free data blocks
	(offset + count) / BLOCK_SIZE > files[fd]->size / BLOCK_SIZE
	get difference between them
	get that many free blocks
	*/
	
	// Holds final buffer
	char bounce[BLOCK_SIZE * ((files[fd]->offset + count) / BLOCK_SIZE + 1)];
	//memset(bounce, '\0', BLOCK_SIZE * ((*file).size / BLOCK_SIZE + 1));
	
	// Holds block specific buffer
	//char block_bounce[BLOCK_SIZE];

	if ((*file).size) {
		int temp_offset = files[fd]->offset;
		fs_lseek(fd, 0);
		fs_read(fd, bounce, BLOCK_SIZE * ((files[fd]->offset + count) / BLOCK_SIZE + 1));
		fs_lseek(fd, temp_offset);
	}

	memcpy(bounce + files[fd]->offset, buf, count);

	printf("%s\n", bounce);

	int diff = (files[fd]->offset + count) / BLOCK_SIZE - (*file).size / BLOCK_SIZE;

	// Runs if appending data
	if (diff > 0 && (*file).size) {
		int prev_data_block = (*file).index;
		int prev_fat_block = prev_data_block / (BLOCK_SIZE / 2);
		int prev_fat_index = prev_data_block % (BLOCK_SIZE / 2);

		for (int i = 0; i < diff; i++) {
			int free_data_block = get_free_data_block();
			int free_fat_block = free_data_block / (BLOCK_SIZE / 2);
			int free_fat_index = free_data_block % (BLOCK_SIZE / 2);

			fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;
			fat_array[prev_fat_block].index[prev_fat_index] = free_data_block;

			prev_fat_block = free_fat_block;
			prev_fat_index = free_fat_index;
		}

		if ((*file).size < files[fd]->offset + count) {
			(*file).size = files[fd]->offset + count;
		}
	}
	else if (diff == 0 && (*file).size) { // Runs if appending but not enough to fill block
		if ((*file).size < files[fd]->offset + count) {
			(*file).size = files[fd]->offset + count;
		}
		else {
			(*file).size += count;
		}
	}
	else if (!(*file).size) { // Runs if no file
		int prev_data_block = get_free_data_block();
		int prev_fat_block = prev_data_block / (BLOCK_SIZE / 2);
		int prev_fat_index = prev_data_block % (BLOCK_SIZE / 2);
		
		fat_array[prev_fat_block].index[prev_fat_index] = FAT_EOC;

		for (int i = 0; i < (int)count / BLOCK_SIZE; i++) {
			int free_data_block = get_free_data_block();
			int free_fat_block = free_data_block / (BLOCK_SIZE / 2);
			int free_fat_index = free_data_block % (BLOCK_SIZE / 2);

			fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;

			fat_array[prev_fat_block].index[prev_fat_index] = free_data_block;

			prev_fat_block = free_fat_block;
			prev_fat_index = free_fat_index;
		}

		(*file).size = count;
		(*file).index = prev_data_block;
	}

	//printf("%s\n", bounce);

	int to_write = (*file).index;

	int inc = 0;
	while (1) {
		char block_bounce[BLOCK_SIZE];
		memset(block_bounce, '\0', BLOCK_SIZE);

		memcpy(block_bounce, bounce + inc * BLOCK_SIZE, BLOCK_SIZE - 1);

		block_write(to_write + sb->data_blk, block_bounce);

		if (get_next_data_block(to_write) == FAT_EOC) {
			break;
		}

		to_write = get_next_data_block(to_write);
		inc++;
	}

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

	// Holds final buffer
	char bounce[BLOCK_SIZE * ((*file).size / BLOCK_SIZE + 1)];
	memset(bounce, '\0', BLOCK_SIZE * ((*file).size / BLOCK_SIZE + 1));

	// Holds block specific buffer
	char block_bounce[BLOCK_SIZE];

	int to_read = (*file).index;
	int inc = 0;

	// Continuously grab data until reaching FAT_EOC
	while ((++inc)) {
		block_read(to_read + sb->data_blk, block_bounce);
		strncat(bounce, block_bounce, BLOCK_SIZE);

		// Break if reaches FAT_EOC
		if (get_next_data_block(to_read) == FAT_EOC) {
			break;
		}

		to_read = get_next_data_block(to_read);

		if (to_read == -1) {
			count = inc * BLOCK_SIZE - files[fd]->offset;
			break;
		}
	}
	
	strncpy(buf, bounce + files[fd]->offset, count);

	files[fd]->offset += count;

	return count;
}

