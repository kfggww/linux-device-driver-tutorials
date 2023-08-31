#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	int devfd = open("/dev/ssd130x0", O_WRONLY);
	if (devfd < 0) {
		perror("fail to open device file");
		return -1;
	}

	int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("192.168.7.1");
	server_addr.sin_port = htons(4567);

	if (connect(server_sockfd, (struct sockaddr *)&server_addr,
		    sizeof(server_addr)) != 0) {
		perror("fail to connect server\n");
		return -1;
	}

	char *frame = malloc(128 * 8);
	int request_next_frame = 1;

	while (request_next_frame) {
		write(server_sockfd, &request_next_frame, sizeof(int));

		int nread = 0;
		int count = 128 * 8;
		char *buf = frame;
		while ((nread = read(server_sockfd, buf, count)) > 0) {
			count -= nread;
			buf += nread;
		}

		if (count != 0) {
			request_next_frame = 0;
		}

		write(devfd, frame, 128 * 8);
	}

	write(server_sockfd, &request_next_frame, sizeof(int));

	free(frame);
	close(server_sockfd);
	close(devfd);

	return 0;
}