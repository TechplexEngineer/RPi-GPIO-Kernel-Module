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
	int ret;
	uint64_t s,e, w=80000;

	fd = open("/dev/systimer", O_RDONLY);
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	ret  = ioctl(fd, SYSTIMER_READ, &s);
	// if (ret < 0) {
	// 	perror("ioctl");
	// 	close(fd);
	// 	return errno;
	// } else {
	// 	printf("Start: %llu\n",s);
	// }

	ret  = ioctl(fd, SYSTIMER_DELAY, &w);
	// if (ret < 0) {
	// 	perror("ioctl");
	// 	close(fd);
	// 	return errno;
	// } else {
	// 	printf("Wait successfull\n");
	// }

	ret  = ioctl(fd, SYSTIMER_READ, &e);
	// if (ret < 0) {
	// 	perror("ioctl");
	// 	close(fd);
	// 	return errno;
	// } else {
	// 	printf("End: %llu\n",e);
	// }
	printf("Start: %llu\n",s);
	printf("End: %llu\n",e);
	printf("Delta: %lld\n", e-s);
	printf("Wait: %lld\n", w);

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

