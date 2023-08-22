#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int result = 0;
volatile int device_fd = -1;

void sigio_handler(int sig)
{
	printf("Reader got signal: %d\n", sig);
	char buf[1024];
	int n = read(device_fd, buf, 1024);
	if (n < 0) {
		perror("read fail");
	} else {
		printf("Reader recv data: %s\n", buf);
	}
	result = 1;
}

int main(int argc, char *argv[])
{
	int ret = 0;

	device_fd = open("/dev/cdev03", O_RDONLY);
	if (device_fd < 0) {
		perror("failed to open device file");
		return -1;
	}

	/* set the signal handler for SIGIO */
	struct sigaction act = {
		.sa_handler = sigio_handler,
		.sa_mask = 0,
		.sa_flags = 0,
	};

	if (sigaction(SIGIO, &act, NULL)) {
		perror("failed to set SIGIO handler");
		ret = -2;
		goto out;
	}

	/* set the owner and fasync flags of device file */
	if (fcntl(device_fd, F_SETOWN, getpid())) {
		perror("failed to set owner of device file");
		ret = -3;
		goto out;
	}
	int flags = fcntl(device_fd, F_GETFL);
	if (fcntl(device_fd, F_SETFL, flags | FASYNC)) {
		perror("failed to set FASYNC flag");
		ret = -4;
		goto out;
	}

	printf("Reader waiting for SIGIO...\n");
	while (result == 0)
		;
	printf("Reader exit\n");
out:
	close(device_fd);
	return ret;
}
