#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FRAME_SIZE (128 * 8)

int main(int argc, char **argv)
{
	int device_fd = open("/dev/ssd130x0", O_WRONLY);
	if (device_fd < 0) {
		return -1;
	}

	char *frame = malloc(FRAME_SIZE);
	memset(frame, 0x88, FRAME_SIZE);
	write(device_fd, frame, FRAME_SIZE);
	sleep(2);

	free(frame);
	close(device_fd);
	return 0;
}