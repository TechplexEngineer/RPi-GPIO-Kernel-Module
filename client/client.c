#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "systimer.h"

int main (int argc, char*argv[])
{
	int fd;
	uint64_t t;
	int ret;
	
	fd = open("/dev/systimer", O_RDONLY);
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	ret  = ioctl(fd, SYSTIMER_READ, &t);
	if (ret < 0) {
		perror("ioclt");
		close(fd);
		return errno;
	} else {
		printf("%llu\n",t);
	}
	
	
/*
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
*/
	close(fd);
	return 0;
}

