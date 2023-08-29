#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int fd = open("/dev/ssd130x0", O_WRONLY);
	if (fd < 0) {
		return -1;
	}

	char *frame = malloc(128 * 8);
	memset(frame, 0, 128 * 8);
	for (int i = 0; i < 64; i++) {
		frame[i] = 0xAA;
	}

	write(fd, frame, 128 * 8);

	sleep(5);
	close(fd);
	return 0;
}