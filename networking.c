#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "test.h"

char networkActive = 0;

struct person {
	int myBox;
	int fd;
};

static struct person phone; // But... WHO WAS PHONE???

void initNetwork(int argc, char **argv) {
	int16_t port = htons(atoi(argv[1]));
	if (*(argv[2]) == 'h') {
		networkActive = 1;
		int listenSock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = port, .sin_addr.s_addr = htonl(INADDR_ANY)};
		bind(listenSock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		listen(listenSock, 1);
		phone.fd = accept(listenSock, NULL, NULL);
		close(listenSock);
	} else {
		if (*(argv[2]) != 'c' || argc < 4) {
			puts("Usage: <port>  < h | c <ip> >");
			return;
		}
		networkActive = 1;
		phone.fd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = port, .sin_addr.s_addr = htonl(INADDR_ANY)};
		bind(phone.fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		addr.sin_addr.s_addr = inet_addr(argv[3]);
		connect(phone.fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	}
	fcntl(phone.fd, F_SETFL, O_NONBLOCK);
	box *tmp = addBox();
	tmp->color = 0x00FFFFFF;
	tmp->size[0] =
	tmp->size[1] =
	tmp->size[2] =
	tmp->size[3] = playerSize;
	tmp->pos[0] =
	tmp->pos[1] =
	tmp->pos[2] =
	tmp->pos[3] = 0;
	phone.myBox = tmp-boxen;
}

void netTick() {
	int32_t pos[4];
	int i = 0;
	for (; i < 4; i++) {
		pos[i] = htonl(me.pos[i]);
	}
	int ret = 0;
	while (ret > -1 && ret < 4*sizeof(int32_t)) {
		//If we don't write all the bytes successfully, TRY AGAIN.
		ret += write(phone.fd, (char*)pos + ret, 4*sizeof(int32_t) - ret);
	}
	static int offset = 0;
	while (1) {
		ret = read(phone.fd, ((char*)pos) + offset, 4*sizeof(int32_t) - offset);
		if (ret == -1) return;
		offset += ret;
		if (offset >= 4*sizeof(int32_t)) {
			int *tmp = boxen[phone.myBox].pos;
			for (i = 0; i < 4; i++) {
				tmp[i] = ntohl(pos[i]);
			}
			if (offset > 4*sizeof(int32_t)) puts("network probs...");
			offset = 0;
		} else {
			return;
		}
	}
}

void stopNetwork() {
	close(phone.fd);
}
