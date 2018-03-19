#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <cstdio>
#include "../Share/Shares.h"
#include "GSServer.h"

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	BOOL isNoDelay{ TRUE };
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay));

	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(GS_PORT);

	bind(sock, (const sockaddr*)&addr, sizeof(sockaddr));
	listen(sock, SOMAXCONN);
	sockaddr clientAddr;
	int addrLen = sizeof(clientAddr);
	auto clientSock = accept(sock, &clientAddr, &addrLen);
	if (clientSock == INVALID_SOCKET) err_quit(TEXT("WSAAccept"));
	printf_s("client has connected\n");

	short dataLen{ 0 };
	ClientMsg msg;
	int cx{ 0 }, cy{ 0 };
	char packetBuf[sizeof(short) + sizeof(ServerMsg) + sizeof(byte) * 2];

	while (true) {
		if (RecvAll(clientSock, (char*)&dataLen, sizeof(dataLen), 0) == 0) break;
		if (RecvAll(clientSock, (char*)&msg, sizeof(msg), 0) == 0) break;
		printf_s("received packet from client\n");

		switch (msg) {
		case ClientMsg::KEY_UP:
			if (cy > 0) cy -= 1;
			SendMovePacket(clientSock, packetBuf, cx, cy);
			break;
		case ClientMsg::KEY_DOWN:
			if (cy < boardH - 1) cy += 1;
			SendMovePacket(clientSock, packetBuf, cx, cy);
			break;
		case ClientMsg::KEY_LEFT:
			if (cx > 0) cx -= 1;
			SendMovePacket(clientSock, packetBuf, cx, cy);
			break;
		case ClientMsg::KEY_RIGHT:
			if (cx < boardW - 1) cx += 1;
			SendMovePacket(clientSock, packetBuf, cx, cy);
			break;
		}
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	if (clientSock) {
		shutdown(clientSock, SD_BOTH);
		closesocket(clientSock);
	}
	WSACleanup();
}

void SendMovePacket(SOCKET sock, char* buf, int x, int y) {
	auto& totalLen = *(short*)buf;
	auto& msg = *(ServerMsg*)(buf + sizeof(totalLen));
	totalLen = sizeof(totalLen) + sizeof(msg) + sizeof(byte) * 2;
	msg = ServerMsg::MOVE_CHARA;
	*(buf + sizeof(totalLen) + sizeof(msg)) = (byte)x;
	*(buf + sizeof(totalLen) + sizeof(msg) + sizeof(byte)) = (byte)y;

	send(sock, (char*)buf, totalLen, 0);
}
