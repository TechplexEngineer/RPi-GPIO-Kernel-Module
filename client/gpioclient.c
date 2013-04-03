#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "modgpio.h"

int main(int argc, char * argv[])
{
	int fd;
	int ret;
	//char * fret=NULL;
	uint32_t v=0;
	//uint64_t c=0;
	char* buf = NULL;

	using_history();

	fd = open("/dev/rpigpio", O_RDONLY); ///hmm, if read only howcome write works?
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	//printf("[I]nsert or [D]elete or [Q]uit. FMT: I 2 \n");
	//printf("[W]ait or [R]ead or [P]rint \n");
	printf("[R]ead [W]rite\n");

	while (1) {
		if (buf != NULL)
			free(buf);
		// Get new item/ delete item
		buf = readline ("Command: ");
		if (buf == NULL) {
			break;	//NULL on eof
		}
		add_history(buf);

		if (buf[1] == ' ')
			v=atoi(&buf[2]);
		else
			v=atoi(&buf[1]);
		
		// Action based on input
		// if ((buf[0]=='i')||(buf[0]=='I')) {
		// 	ret = ioctl(fd, SYSTIMER_ll_INS, &v);
		// 	if (ret < 0) {
		// 		perror("ioctl");
		// 	}
		// } else if ((buf[0]=='d')||(buf[0]=='D')) {
		// 	ret = ioctl(fd, SYSTIMER_ll_DEL, &v);
		// 	if (ret < 0) {
		// 		perror("ioctl");
		// 	}
		// } else if ((buf[0]=='w')||(buf[0]=='W')) {
		// 	ret = ioctl(fd, SYSTIMER_DELAY, &v);
		// 	if (ret < 0) {
		// 		perror("ioctl");
		// 	}
		// 	printf("Wait: %lu\n", v);
		// } else 
		if ((buf[0]=='r')||(buf[0]=='R')) {
			int pin = 17;
			ret = ioctl(fd, GPIO_READ, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog\n");
		} else 
		if ((buf[0]=='w')||(buf[0]=='w')) {
			printf("v: %d\n",v);
			int pin = 17;
			struct gpio_data_write mystruct = {
				.pin = 17,
				.data = v,
			};
			ret = ioctl(fd, GPIO_WRITE, &mystruct);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog\n");
		} else
		// if((buf[0]=='p')||(buf[0]=='P')) {
		// 	// Print the list
		// 	ret = ioctl(fd, SYSTIMER_ll_PRINT);
		// 	if (ret < 0) {
		// 		perror("ioctl");
		// 	}
		// } else 
		if ((buf[0]=='q')||(buf[0]=='Q')) {
			break;
		} else {
			printf("Unknown Command \n");
		}
	}
	clear_history();
	printf("\n");
	close(fd);
	return 0;
}
