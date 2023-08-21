#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	// open device file in block mode
	int dev_fd = open("/dev/cdev02", O_RDWR);
	if (dev_fd < 0) {
		perror("Can not open device file");
		return -1;
	}

	pid_t reader_pid = getpid();

	if (fork() == 0) {
		for (int i = 1; i < argc; i++) {
			write(dev_fd, argv[i], strlen(argv[i]));
			sleep(1);
		}
		kill(reader_pid, SIGINT);
		printf("Writer send SIGINT, exit\n");
		exit(0);
	}

	// reader process
	char buf[1024];
	memset(buf, 0, 1024);
	while (read(dev_fd, buf, 1024) > 0) {
		printf("Reader recv data: %s\n", buf);
		memset(buf, 0, 1024);
	}

	printf("Reader exit\n");
	return 0;
}
