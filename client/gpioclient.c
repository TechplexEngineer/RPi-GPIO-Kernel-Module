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
	int v=0;
	int pin = 17;
	//uint64_t c=0;
	char* buf = NULL;
	char *found = NULL;

	using_history();

	fd = open("/dev/rpigpio", O_RDONLY); ///hmm, if read only howcome write works?
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	//printf("[I]nsert or [D]elete or [Q]uit. FMT: I 2 \n");
	//printf("[W]ait or [R]ead or [P]rint \n");
	printf("[R]ead [W]rite [C]heckout [F]ree\n");

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

		if ((buf[0]=='r')||(buf[0]=='R')) {
			//int pin = 17;
			ret = ioctl(fd, GPIO_READ, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog %d\n", pin);
		} else
		if ((buf[0]=='w')||(buf[0]=='W')) {

			if (buf[1] == ' ') //this causes segfault when second param not provided
				found = strstr(&buf[3], " ");
			else
				found = strstr(&buf[2], " ");
			if (found == NULL) {
				printf("Missing 2nd parameter. Usage \"w <val> <pin>\"\n");
				continue;
			}

			pin = atoi(found);//17;//get from user
			printf("v:%d pin:%d\n",v, pin);

			struct gpio_data_write mystruct = {
				.pin = pin, //ideally we'd get both from the user
				.data = v,
			};
			ret = ioctl(fd, GPIO_WRITE, &mystruct);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog\n");
		} else
		if ((buf[0]=='c')||(buf[0]=='C')) {
			printf("v: %d\n",v);
			pin = v;
			ret = ioctl(fd, GPIO_REQUEST, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog\n");
		} else
		if ((buf[0]=='f')||(buf[0]=='F')) {
			printf("v: %d\n",v);
			pin = v;
			ret = ioctl(fd, GPIO_FREE, &pin);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("check syslog\n");
		} else
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
// if((buf[0]=='p')||(buf[0]=='P')) {
		// 	// Print the list
		// 	ret = ioctl(fd, SYSTIMER_ll_PRINT);
		// 	if (ret < 0) {
		// 		perror("ioctl");
		// 	}
		// } else 