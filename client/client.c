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
	fd = open("/dev/systimer", O_RDONLY);

	if (fd < 0) {
		perror("open");
		return errno;
	}
	sleep(20);
	close(fd);
	return 0;
}

