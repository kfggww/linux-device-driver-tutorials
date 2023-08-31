#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#define TARGET_WIDTH 128
#define TARGET_HEIGHT 64
#define TARGET_SIZE (TARGET_WIDTH * TARGET_HEIGHT / 8)

extern void resize_frame(unsigned char *oframe, int ow, int oh,
				   unsigned char *nframe, int nw, int nh,
				   int threshold);

int main(int argc, char **argv)
{
	int threshold = 200;
	if (argc >= 2) {
		threshold = atoi(argv[1]);
	}

	/* start local server */
	int local_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(4567);

	bind(local_server_sockfd, (struct sockaddr *)&local_addr,
	     sizeof(local_addr));
	listen(local_server_sockfd, 10);

	// wait the connection from client
	struct sockaddr_in cli;
	socklen_t len;
	int client_sockfd =
		accept(local_server_sockfd, (struct sockaddr *)&cli, &len);
	int client_request_next_frame = 0;
	read(client_sockfd, &client_request_next_frame, sizeof(int));
	if (client_request_next_frame == 0) {
		close(client_sockfd);
		goto out;
	}

	int remote_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* connect to video server */
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(3456);

	if (connect(remote_server_sockfd, (struct sockaddr *)&server_addr,
		    sizeof(server_addr))) {
		perror("fail to connect server");
		return -1;
	}

	// read width, height from video server
	int width;
	int height;
	size_t orig_frame_size;
	unsigned char *orig_frame = NULL;
	unsigned char *resized_frame = NULL;

	read(remote_server_sockfd, &width, 4);
	read(remote_server_sockfd, &height, 4);
	width = ntohl(width);
	height = ntohl(height);

	printf("width=%d, height=%d\n", width, height);

	orig_frame_size = width * height * 3;
	orig_frame = malloc(orig_frame_size);
	resized_frame = malloc(TARGET_SIZE);

	// request next frame from video server
	int request_next_frame = 1;
	write(remote_server_sockfd, &request_next_frame,
	      sizeof(request_next_frame));

	while (1) {
		// read original fram from video server
		char *buf = orig_frame;
		int nread = 0;
		size_t count = orig_frame_size;
		while (count > 0 &&
		       (nread = read(remote_server_sockfd, buf, count)) > 0) {
			buf += nread;
			count -= nread;
		}

		if (count != 0) {
			break;
		}

		// get resized frame using GPU
		resize_frame(orig_frame, width, height, resized_frame,
				       TARGET_WIDTH, TARGET_HEIGHT, threshold);

		// send resized frame to bbb board
		write(client_sockfd, resized_frame, TARGET_SIZE);

		/* wait the client on bbb board finish his job */
		client_request_next_frame = 0;
		read(client_sockfd, &client_request_next_frame, sizeof(int));
		if (client_request_next_frame == 0) {
			close(client_sockfd);
			break;
		}

		/* request next original frame */
		write(remote_server_sockfd, &request_next_frame,
		      sizeof(request_next_frame));
	}

	free(orig_frame);
	free(resized_frame);
	close(remote_server_sockfd);
out:
	close(local_server_sockfd);
	return 0;
}