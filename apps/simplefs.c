#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fs.h>

int main(int argc, char** argv){
	printf("Mounting Disk %s \n", argv[1]);
	fs_mount(argv[1]);
	fs_info();
	fs_umount();

	return 0;
}