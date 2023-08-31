#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int device_fd = open("/dev/ssd130x0", O_WRONLY);
	if (device_fd < 0) {
		return -1;
	}

	int video_fd = open(argv[1], O_RDONLY);
	if (video_fd < 0) {
		return -1;
	}

	char *frame = malloc(128 * 8);
	memset(frame, 0, 128 * 8);

	while (read(video_fd, frame, 128 * 8) > 0) {
		write(device_fd, frame, 128 * 8);
	}

	close(video_fd);
	close(device_fd);
	return 0;
}