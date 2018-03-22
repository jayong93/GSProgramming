#include "Shares.h"

void err_quit(LPCTSTR msg) {
	LPVOID msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)msgBuf, msg, MB_ICONERROR);
	LocalFree(msgBuf);
	exit(1);
}

