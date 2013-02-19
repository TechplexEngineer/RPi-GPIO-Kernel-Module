#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main (int argc, char*argv[])
{
	int fd;
	fd = open("/dev/systimer", O_RDWR);
	if(fd) {
		perror("open(O_RDWR)");
	} else {
		close(fd);
	}

	fd = open("/dev/systimer", O_WRONLY);
	if(fd) {
		perror("open(O_WRONLY)");
	} else {
		close(fd);
	}

	fd = open("/dev/systimer", O_RDONLY);
	if(fd) {
		perror("open(O_RDONLY)");
	} else {
		close(fd);
	}
	return 0;
}

