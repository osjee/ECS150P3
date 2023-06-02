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
	//printf("Mounting Disk %s \n", argv[1]);
	fs_mount(argv[1]);
	//fs_info();

	char *buf = malloc(9999);
	char* buf2 = malloc(9999);
	//char* buf3 = malloc(9999);

	int fd;

	fs_create("Hello.txt");
	fd = fs_open("Hello.txt");
	fs_write(fd, "99999", 5);
	fs_write(fd, "11111", 5);
	fs_lseek(fd, 0);
	fs_read(fd, buf, 5);
	fs_read(fd, buf2, 5);
	printf("%s\n", buf);
	printf("%s\n", buf2);

	fs_ls();

	/*
	for (int i = 0; i < 50; i++) {
		fs_create("file0");
		printf("%d\n",fs_open("file0"));
	}
	*/
	//int fd;
	//int fd2;
	//int fd3; 

	// fd = fs_open("b.txt");
	// fs_read(fd, buf, fs_stat(fd));
	// printf("%s\n", buf);
	// fs_close(fd);

	/*
	char *file0 = "test_file0";
	char* file1 = "test_file1";
	char* file2 = "test_file2";
	//char *text0 = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz==================================================================+4096";
	fs_create(file0);
	fs_create(file1);
	fs_create(file2);
		fd = fs_open(file0);
		fd2 = fs_open(file1);
		fd3 = fs_open(file2);

			printf("wrote %d bytes\n",fs_write(fd, "99999", 5));
			printf("wrote %d bytes\n", fs_write(fd2, "89999", 5));
			printf("wrote %d bytes\n", fs_write(fd3, "79999", 5));
			//fs_lseek(fd, 5);
			//printf("wrote %d bytes\n", fs_write(fd, text0, strlen(text0)));
			//fs_lseek(fd, 5 + strlen(text0));
			//printf("wrote %d bytes\n", fs_write(fd, text0, strlen(text0)));
			fs_lseek(fd, 0);
			fs_lseek(fd2, 0);
			fs_lseek(fd3, 0);
			printf("read %d bytes\n", fs_read(fd, buf, 5 ));
			printf("read %d bytes\n", fs_read(fd2, buf2, 5));
			printf("read %d bytes\n", fs_read(fd3, buf3, 5));


			//fs_lseek(fd, 0);
			printf("%s\n", buf);
			printf("%s\n", buf2);
			printf("%s\n", buf3);
			//fs_ls();
			//fs_stat(fd);

		fs_close(fd);
		fs_close(fd2);
		fs_close(fd3);

		*/
	//fs_delete(file0);
	//char *file1 = "test_file1";
	// char *text1 = "this is another test file but longer than the first one";
	// fs_create(file1);		
	// 	fd = fs_open(file1);
	// 		fs_write(fd, text1, strlen(text1));
	// 		//fs_read(fd, buf, strlen(text1));
	// 		fs_lseek(fd, fs_stat(fd));
	// 		fs_write(fd, text1, strlen(text1));
	// 		fs_lseek(fd, 0);
	// 		fs_read(fd, buf, 1000);
	// 		fs_lseek(fd, 0);
	// 		fs_read(fd, buf, 1000);
	// 		printf("%s\n", buf);
	// 	fs_close(fd);
	// fs_delete(file1);
	// fs_create("abcdefg");
	// 	fd = fs_open("abcdefg");
	// 		fs_write(fd, "abcdefg", 7);
	// 		fs_read(fd, buf, 7);
	// 		printf("%s\n", buf);
	// 	fs_close(fd);
	// fs_delete("abcdefg");
	// fs_create("hijklmn");
	// 	fd = fs_open("hijklmn");
	// 		fs_write(fd, "hijklmn", 7);
	// 		fs_read(fd, buf, 7);
	// 		printf("%s\n", buf);
	// 	fs_close(fd);
	// fs_delete("hijklmn");
	
	fs_umount();

	free(buf);
	buf = NULL;

	return 0;
}