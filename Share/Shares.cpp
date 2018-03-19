#include "Shares.h"

void err_quit(LPCTSTR msg) {
	LPVOID msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)msgBuf, msg, MB_ICONERROR);
	LocalFree(msgBuf);
	exit(1);
}

int RecvAll(SOCKET sock, char * buf, size_t len, int flag) {
	int receivedLen{ 0 };
	int retval;
	while (receivedLen < len) {
		if ((retval = recv(sock, buf + receivedLen, len - receivedLen, flag)) == SOCKET_ERROR) err_quit(TEXT("recv"));
		if (retval == 0) return 0;
		receivedLen += retval;
	}
	return receivedLen;
}
