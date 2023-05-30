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

	return sb->data_blk + index + (block * BLOCK_SIZE / 2);
}

// Returns entry of FAT from data block
int get_fat_entry(int db) {
	int fat_block = (db) / (BLOCK_SIZE / 2);
	int fat_index = (db) % (BLOCK_SIZE / 2);

	return fat_array[fat_block].index[fat_index];
}

// Returns index of next data block
int get_next_data_block(int db) {
	return get_fat_entry(db);
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
	int to_delete = rd->entries[found_index].index;

	// Remove data from FAT array
	while (1) {
		// Chooses the block to delete from
		int fat_block = (to_delete - sb->data_blk) / (BLOCK_SIZE / 2);
		// Chooses the index of the block to delete from
		int fat_index = (to_delete - sb->data_blk) % (BLOCK_SIZE / 2);

		if (get_fat_entry(to_delete) == FAT_EOC) {
			fat_array[fat_block].index[fat_index] = 0;
			break;
		}

		fat_array[fat_block].index[fat_index] = 0;

		int to_delete = get_next_data_block(to_delete);
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

//Will write the first bit of count to fill the block from the offset
//Returns the index of the new data block if count overflowed the data block
//If not filled it returns 0
int write_first(int fd, void* buf, size_t count) {

	//The offset from the beggining of the block
	int offset_in_file = files[fd]->offset % BLOCK_SIZE;

	//Start index of fat entry that contains the offset
	int offset_data_entry = rd->entries[files[fd]->root_entry].index;

	//Find fat block that contains the entry of intrest
	int fat_block = files[fd]->offset / BLOCK_SIZE;

	//Find fat entry that contains offset
	for (int i = 0; i < offset_in_file; i++) {
		offset_data_entry = fat_array[fat_block].index[offset_data_entry];
	}

	//Check to see if count if bigger than whats avaliable
	size_t ammount = count;
	size_t avaliable = BLOCK_SIZE - offset_in_file;

	if (avaliable < count) {
		ammount = avaliable;
	}

	//Holds enough for the number of characters in the data block + the amount to add
	char bounce[ammount+ offset_in_file];

	//Copy values over if exist in data block
	if (offset_in_file != 0) {
		char read_in[offset_in_file];

		//Read the beggining block into read_in
		block_read(offset_data_entry + sb->data_blk, read_in);

		strncpy(bounce, read_in, offset_in_file);
	}

	//Overwrite empty spots with enough from buffer to fill the block
	strncpy(&bounce[offset_in_file], buf, ammount);

	//Write back to the file
	block_write(offset_data_entry + sb->data_blk, bounce);

	//Move offset
	files[fd]->offset = files[fd]->offset + ammount;

	//If it filled the block make another data block and add it to the entry
	if (ammount != count) {

		printf("entered zone \n");

		//Get a new data block 
		int new_data_block = get_free_data_block();

		//Find fat index of that data block
		int new_fat_block = (new_data_block - sb->data_blk) / (BLOCK_SIZE / 2);
		int new_fat_index = (new_data_block - sb->data_blk) % (BLOCK_SIZE / 2);

		//Make old fat entry point to newer data block
		fat_array[fat_block].index[offset_data_entry] = new_data_block;

		//Make new fat entry point to FAT_EOC
		fat_array[new_fat_block].index[new_fat_index] = FAT_EOC;

		//Return index of new data block
		return new_data_block;
	}

	//Change file size to match new write
	rd->entries[files[fd]->root_entry].size += ammount;

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

	//struct root_dir_entry *file = &rd->entries[files[fd]->root_entry];

	// Get locations of data and FAT to update

	int free_data_block = get_free_data_block();
	int free_fat_block = (free_data_block - sb->data_blk) / (BLOCK_SIZE / 2);
	int free_fat_index = (free_data_block - sb->data_blk) % (BLOCK_SIZE / 2);

	//char bounce[count];

	// Holds block specific buffer
	char block_bounce[BLOCK_SIZE];

	//Create a data block if the root is FAT_EOC
	if (rd->entries[files[fd]->root_entry].index == FAT_EOC) {
		
		rd->entries[files[fd]->root_entry].index = free_data_block - sb->data_blk;
		fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;

	}

	//Fill first bit of the block
	int data_block = write_first(fd, buf, count);

	//Check to see if the block was filled
	if (data_block == 0) {
		return strlen(buf);
	}

	printf("You should not get here with a simple test \n");
		
	//If not run through all blocks until last one
	//The range is how many FULL blocks we can fill, we have to subtract the amount done by write_first
	int ammount = count - (BLOCK_SIZE - files[fd]->offset);

	for (int i = 0; i < ammount / BLOCK_SIZE; i++) {
			
		//Move each chunk into bounce
		strncpy(block_bounce, buf + (BLOCK_SIZE - files[fd]->offset) + BLOCK_SIZE * i, BLOCK_SIZE);
			
		//Write bounce to desired block
		block_write(sb->data_blk+data_block, block_bounce);

		//Check if there is no other block to traverse too
		if (get_fat_entry(data_block) == FAT_EOC) {

			//Make new entry
			free_data_block = get_free_data_block();
			free_fat_block = (free_data_block - sb->data_blk) / (BLOCK_SIZE / 2);
			free_fat_index = (free_data_block - sb->data_blk) % (BLOCK_SIZE / 2);
			fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;

			//Change old entry to point towards new entry
			int current_fat_block = (data_block - sb->data_blk) / (BLOCK_SIZE / 2);
			int current_fat_index = (data_block - sb->data_blk) % (BLOCK_SIZE / 2);
			fat_array[current_fat_block].index[current_fat_index] = free_data_block;

			//Change the old data block into the new one
			data_block = free_data_block;
		}
		else {
			//Change the old data block into the new one
			data_block = sb->data_blk + get_fat_entry(data_block);
		}

		//Move offset along
		files[fd]->offset += BLOCK_SIZE;
	}
		
	//Return if it was a perfect fit
	if (ammount % BLOCK_SIZE == 0) {
		return strlen(buf);
	}

	//After finishing the chunks write the last part
	strncpy(block_bounce, buf + count - ((count - ammount) % BLOCK_SIZE), (count - ammount) % BLOCK_SIZE);
	block_write(data_block, block_bounce);

	files[fd]->offset += (count - ammount) % BLOCK_SIZE;
	//Change this to less if it fails to write the entirety of count
	rd->entries[files[fd]->root_entry].size = count;

	/*
	strncpy(bounce, buf, count);

	block_write(free_data_block, bounce);

	// Update FAT array
	fat_array[free_fat_block].index[free_fat_index] = FAT_EOC;
	block_write(free_fat_block + 1, &fat_array[free_fat_block]);

	// Update root directory entry's size and index of data block
	(*file).size = count; // strlen(count) maybe?
	(*file).index = free_data_block;

	//Change offset
	files[fd]->offset = count;

	block_write(sb->rdir_blk, rd);
	*/
	
	//NEED TO CHANGE THIS IF DISK RUNS OUT OF DATA BLOCKS TO WRITE TO
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

	int data_block_to_read = (*file).index;

	// Continuously grab data until reaching FAT_EOC
	while (1) {
		block_read(data_block_to_read + sb->data_blk, block_bounce);
		strcat(bounce, block_bounce);

		// Break if reaches FAT_EOC
		if (get_fat_entry(data_block_to_read) == FAT_EOC) {
			break;
		}
		data_block_to_read = get_next_data_block(data_block_to_read);
	}
	strncpy(buf, bounce + files[fd]->offset, count);
	return 0;
}

