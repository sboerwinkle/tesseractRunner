#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "test.h"
#include "networking.h"

char networkActive = 0;

#define CLIENT 1
#define HOST 2

typedef struct {
	int a, b; //The box IDs of their body and head
} avatar;

static avatar *avatars;
static int numAvatars = 0;

typedef struct {
	int fd;
	int netGarbage;
} client;

static client *clients;

static client server = {0};
int whichAvatar = -1;

static void initAvatar(avatar *a) {
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
	a->a = tmp-boxen;
	tmp = addBox();
	tmp->color = 0x00FFFFFF;
	tmp->size[0] =
	tmp->size[1] =
	tmp->size[2] =
	tmp->size[3] = playerSize/5;
	tmp->pos[0] = playerSize * 2;
	tmp->pos[1] =
	tmp->pos[2] =
	tmp->pos[3] = 0;
	a->b = tmp-boxen;
}

void initNetwork(int argc, char **argv) {
	int16_t port = htons(atoi(argv[1]));
	if (*(argv[2]) == 'h') {
		networkActive = HOST;
		numAvatars = atoi(argv[3]);
		clients = calloc(numAvatars, sizeof(client));
		int listenSock = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = port, .sin_addr.s_addr = htonl(INADDR_ANY)};
		bind(listenSock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		listen(listenSock, numAvatars);
		int i = 0;
		unsigned char num = numAvatars;
		for (; i < numAvatars; i++) {
			int fd = accept(listenSock, NULL, NULL);
			clients[i].fd = fd;
			fcntl(fd, F_SETFL, O_NONBLOCK);
			if (1 != write(fd, &num, 1)) puts("What what wh");
		}
		close(listenSock);
	} else {
		if (*(argv[2]) != 'c') {
			puts(USAGE_MSG);
			return;
		}
		networkActive = CLIENT;
		server.fd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = port, .sin_addr.s_addr = htonl(INADDR_ANY)};
		bind(server.fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		addr.sin_addr.s_addr = inet_addr(argv[3]);
		connect(server.fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		unsigned char num;
		if (1 != read(server.fd, &num, 1)) puts("Wait, wha");
		numAvatars = num;
		fcntl(server.fd, F_SETFL, O_NONBLOCK);
	}
	avatars = malloc(sizeof(avatar) * numAvatars);
	int i = 0;
	for (; i < numAvatars; i++)
		initAvatar(avatars + i);
}

#define packSize (8*sizeof(int32_t))
static int32_t pos[8];

static char flushGarbage(client* who) {
	int ret;
	while (who->netGarbage) {
		ret = read(who->fd, pos, who->netGarbage);
		if (ret == -1) return 1;
		who->netGarbage -= ret;
	}
	return 0;
}

static void sendPack(int fd) {
	int ret, offset = 0;
	while (offset < packSize) {
		ret = write(fd, (char*)pos + offset, packSize - offset);
		if (ret == -1) {
			if (offset) continue; //If we don't write all the bytes successfully, TRY AGAIN.
			else return;
		}
		offset += ret;
	}
}

static void loadMyData() {
	int i = 0;
	for (; i < 4; i++) {
		pos[i] = htonl(me.pos[i]);
		pos[4+i] = htonl(playerSize * 2 * me.view[0][i]);
	}
}

static char readPack(client *cli, avatar *a) {
	int ret, offset = 0;
	do {
		ret = read(cli->fd, (char*)pos + offset, packSize - offset);
	} while(ret != -1 && (offset += ret) != packSize);
	if (ret == -1) {
		cli->netGarbage = packSize - offset;
		return 1;
	}
	int *tmp  = boxen[a->a].pos;
	int *tmp2 = boxen[a->b].pos;
	int i = 0;
	for (; i < 4; i++) {
		tmp[i] = ntohl(pos[i]);
		tmp2[i] = tmp[i] + ntohl(pos[4+i]);
	}
	return 0;
}

static void netTickClient() {
	loadMyData();
	sendPack(server.fd);

	//if (flushGarbage(&server)) return; //If there's still junk to read, I'd better not try to interpret it

	do {
		whichAvatar = (whichAvatar+1) % numAvatars;
	} while (!readPack(&server, avatars+whichAvatar));
}

static void netTickHost() {
	loadMyData();
	int i = 0;
	for (; i < numAvatars; i++)
		sendPack(clients[i].fd);
	int j;
	for (i = 0; i < numAvatars; i++) {
		for (j = 0; j < 4; j++) {
			pos[i] = htonl(boxen[avatars[i].a].pos[i]);
			pos[4+i] = htonl(boxen[avatars[i].b].pos[i] - pos[i]);
		}
		for (j = 0; j < numAvatars; j++) {
			if (j==i) continue; // No one needs to know where they themselves are
			sendPack(clients[j].fd);
		}
	}

	for (i = 0; i < numAvatars; i++) {
		//if (flushGarbage(clients + i)) continue;
		while (!readPack(clients + i, avatars + i));
	}
}

void netTick() {
	if (networkActive == HOST) netTickHost();
	else netTickClient();
}

void stopNetwork() {
	if (networkActive == HOST) {
		int i = 0;
		for (; i < numAvatars; i++)
			close(clients[i].fd);
		free(clients);
	} else {
		close(server.fd);
	}
	free(avatars);
}
