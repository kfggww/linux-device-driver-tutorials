#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_THRESHOLD 128
#define TARGET_WIDTH 128
#define TARGET_HEIGHT 64
#define TARGET_SIZE (TARGET_WIDTH * TARGET_HEIGHT / 8)

extern void resize_frame(unsigned char *oframe, int ow, int oh,
			 unsigned char *nframe, int nw, int nh, int threshold);

int main(int argc, char **argv)
{
	int threshold = DEFAULT_THRESHOLD;
	if (argc >= 2) {
		threshold = atoi(argv[1]);
	}

	/* Listen on port 4567, wait connection from beagle board */
	int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(4567);

	bind(listen_sockfd, (struct sockaddr *)&listen_addr,
	     sizeof(listen_addr));
	listen(listen_sockfd, 10);

	int down_connection_sockfd = accept(listen_sockfd, NULL, NULL);
	close(listen_sockfd);

	int request_next_frame = 0;
	read(down_connection_sockfd, &request_next_frame, sizeof(int));
	if (request_next_frame == 0) {
		close(down_connection_sockfd);
		return -1;
	}

	/* Create tcp connection to 127.0.0.1:3456 */
	int up_connection_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(3456);

	if (connect(up_connection_sockfd, (struct sockaddr *)&server_addr,
		    sizeof(server_addr))) {
		close(up_connection_sockfd);
		close(down_connection_sockfd);
		return -2;
	}

	// read width, height from up_connection
	int width;
	int height;
	size_t video_frame_size;
	unsigned char *video_frame = NULL;
	unsigned char *resized_frame = NULL;

	read(up_connection_sockfd, &width, 4);
	read(up_connection_sockfd, &height, 4);
	width = ntohl(width);
	height = ntohl(height);

	printf("video frame size: width=%d, height=%d\n", width, height);

	video_frame_size = width * height * 3;
	video_frame = malloc(video_frame_size);
	resized_frame = malloc(TARGET_SIZE);

	write(up_connection_sockfd, &request_next_frame,
	      sizeof(request_next_frame));

	while (request_next_frame) {
		// read video_frame from up_connection
		char *buf = video_frame;
		int nread = 0;
		size_t count = video_frame_size;
		while (count > 0 &&
		       (nread = read(up_connection_sockfd, buf, count)) > 0) {
			buf += nread;
			count -= nread;
		}

		if (count != 0) {
			break;
		}

		// resize video frame using CUDA
		resize_frame(video_frame, width, height, resized_frame,
			     TARGET_WIDTH, TARGET_HEIGHT, threshold);

		// send resized_frame
		write(down_connection_sockfd, resized_frame, TARGET_SIZE);

		// check if beagle board requests next frame
		request_next_frame = 0;
		read(down_connection_sockfd, &request_next_frame, sizeof(int));

		// request next video frame from up_connection
		write(up_connection_sockfd, &request_next_frame,
		      sizeof(request_next_frame));
	}

	free(video_frame);
	free(resized_frame);
	close(up_connection_sockfd);
	close(down_connection_sockfd);
	return 0;
}