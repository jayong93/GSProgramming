#include <stdafx.h>
#include "Shares.h"

void err_quit_wsa(LPCTSTR msg) {
	err_quit_wsa(WSAGetLastError(), msg);
}

void err_quit_wsa(DWORD errCode, LPCTSTR msg)
{
	LPVOID msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)msgBuf, msg, MB_ICONERROR);
	LocalFree(msgBuf);
	WSACleanup();
	exit(1);
}

void print_network_error(DWORD errCode)
{
	LPVOID msgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, NULL);
	wprintf_s(L"%s", (LPCTSTR)msgBuf);
	LocalFree(msgBuf);
}
