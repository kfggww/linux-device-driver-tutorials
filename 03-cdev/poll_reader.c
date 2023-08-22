#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[])
{
	int dev_fd = open("/dev/cdev03", O_RDWR);
	if (dev_fd < 0) {
		perror("Can not open device file");
		return -1;
	}

	struct pollfd pollfd = {
		.fd = dev_fd,
		.events = POLL_IN,
		.revents = 0,
	};

	char buf[1024];
	int max_poll_calls = 3;
	while (max_poll_calls) {
		int ret = poll(&pollfd, 1, -1);
		if (ret == 1) {
			memset(buf, 0, 1024);
			read(dev_fd, buf, 1024);
			printf("poll_reader recv data: %s\n", buf);
		}
		max_poll_calls--;
	}

	close(dev_fd);
	return 0;
}
