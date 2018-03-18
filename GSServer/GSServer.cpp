#include <WinSock2.h>
#include <cstdio>
#include "../Share/Shares.h"

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	auto sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	BOOL isNoDelay{ TRUE };
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay));

	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(GS_PORT);

	bind(sock, (const sockaddr*)&addr, sizeof(sockaddr));
	listen(sock, 5);
	sockaddr clientAddr;
	int addrLen;
	auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);

	int retval{ 0 };
	short dataLen{ 0 };
	ClientMsg msg;
	int cx{ 0 }, cy{ 0 };

	while (true) {
		if ((retval = RecvAll(sock, (char*)&dataLen, sizeof(dataLen), 0)) == SOCKET_ERROR) break;
		if ((retval = RecvAll(sock, (char*)&msg, sizeof(msg), 0)) == SOCKET_ERROR) break;

		// TODO: 키 입력에 따라 좌표 변경하고(충돌체크도 수행) 클라이언트에게 메세지 전송.
		switch (msg) {
		case ClientMsg::KEY_UP:
			break;
		case ClientMsg::KEY_DOWN:
			break;
		case ClientMsg::KEY_LEFT:
			break;
		case ClientMsg::KEY_RIGHT:
			break;
		}
	}

	WSACleanup();
}