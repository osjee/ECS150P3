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

	char *file0 = "test_file0";
	char *file1 = "test_file1";
	char *text0 = "this is a test file";
	char *text1 = "this is another test file but longer than the first one";
	char *buf = malloc(1024);
	int fd;
	
	fs_create(file0);
	fs_create(file1);
	fs_create("hi");

		fd = fs_open(file0);

			fs_write(fd, text0, strlen(text0));
			fs_read(fd, buf, strlen(text0));

			printf("%s\n", buf);

		fs_close(fd);
		
		fd = fs_open(file1);

			fs_write(fd, text1, strlen(text1));
			fs_lseek(fd, fs_stat(fd));
			fs_write(fd, "bruh", 4);
			fs_lseek(fd, 0);
			fs_read(fd, buf, strlen(text1));

			printf("%s\n", buf);

		fs_close(fd);

		fd = fs_open("hi");

			fs_write(fd, "hi", 2);
			fs_read(fd, buf, 100);

			printf("%s\n", buf);

		fs_close(fd);

	fs_delete(file0);
	fs_delete(file1);
	fs_delete("hi");

	fs_umount();

	free(buf);
	buf = NULL;

	return 0;
}