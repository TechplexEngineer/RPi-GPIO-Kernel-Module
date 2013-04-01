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


#include "systimer.h"

int main(int argc, char * argv[])
{
	int fd;
	int ret;
	//char * fret=NULL;
	uint32_t v=0;
	uint64_t c=0;
	char* buf = NULL;

	using_history();

	fd = open("/dev/systimer", O_RDONLY);
	if(!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	printf("[I]nsert or [D]elete or [Q]uit. FMT: I 2 \n");
	printf("[W]ait or [R]ead or [P]rint \n");

	while (1) {
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
		if ((buf[0]=='i')||(buf[0]=='I')) {
			ret = ioctl(fd, SYSTIMER_ll_INS, &v);
			if (ret < 0) {
				perror("ioctl");
			}
		} else if ((buf[0]=='d')||(buf[0]=='D')) {
			ret = ioctl(fd, SYSTIMER_ll_DEL, &v);
			if (ret < 0) {
				perror("ioctl");
			}
		} else if ((buf[0]=='w')||(buf[0]=='W')) {
			ret = ioctl(fd, SYSTIMER_DELAY, &v);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("Wait: %lu\n", v);
		} else if ((buf[0]=='r')||(buf[0]=='R')) {
			ret = ioctl(fd, SYSTIMER_READ, &c);
			if (ret < 0) {
				perror("ioctl");
			}
			printf("Count is: %llu\n", c);
		} else if((buf[0]=='p')||(buf[0]=='P')) {
			// Print the list
			ret = ioctl(fd, SYSTIMER_ll_PRINT);
			if (ret < 0) {
				perror("ioctl");
			}
		} else if ((buf[0]=='q')||(buf[0]=='Q')) {
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

// int main2 (int argc, char*argv[])
// {
// 	int fd;
// 	int ret;
// 	uint64_t s,e, w=80000;

// 	//SECTION 2
// 	uint64_t first;

// 	//SECTION 1

// 	fd = open("/dev/systimer", O_RDONLY);
// 	if(!fd) {
// 		perror("open(O_RDONLY)");
// 		return errno;
// 	}

// 	ret  = ioctl(fd, SYSTIMER_READ, &s);
// 	// if (ret < 0) {
// 	// 	perror("ioctl");
// 	// 	close(fd);
// 	// 	return errno;
// 	// } else {
// 	// 	printf("Start: %llu\n",s);
// 	// }

// 	ret  = ioctl(fd, SYSTIMER_DELAY, &w);
// 	// if (ret < 0) {
// 	// 	perror("ioctl");
// 	// 	close(fd);
// 	// 	return errno;
// 	// } else {
// 	// 	printf("Wait successfull\n");
// 	// }

// 	ret  = ioctl(fd, SYSTIMER_READ, &e);
// 	// if (ret < 0) {
// 	// 	perror("ioctl");
// 	// 	close(fd);
// 	// 	return errno;
// 	// } else {
// 	// 	printf("End: %llu\n",e);
// 	// }
// 	printf("Start: %llu\n",s);
// 	printf("End: %llu\n",e);
// 	printf("Delta: %lld\n", e-s);
// 	printf("Wait: %lld\n", w);

// 	//SECTION 2
// 	printf("\n\n");

// 	ret  = ioctl(fd, SYSTIMER_LL_FIRST, &first);
// 	printf("first: %lld\n", first);

// 	ret  = ioctl(fd, SYSTIMER_ll_PRINT);
// 	printf("printed\n");

// 	ret = ioctl(fd, SYSTIMER_ll_INS, &first);

// 	ret  = ioctl(fd, SYSTIMER_ll_DEL, &first);


// /*
// 	fd = open("/dev/systimer", O_RDWR);
//     if(fd) {
//         perror("open(O_RDWR)");
//     } else {
//         close(fd);
//     }

//     fd = open("/dev/systimer", O_WRONLY);
//     if(fd) {
//         perror("open(O_WRONLY)");
//     } else {
//         close(fd);
//     }
// */
// 	close(fd);
// 	return 0;
// }

