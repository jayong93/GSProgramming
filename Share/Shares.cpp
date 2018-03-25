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

void MsgReconstructor::Recv(SOCKET s)
{
	int retval = recv(s, (char*)buf.data() + bufSize, bufMaxLen - bufSize, 0);
	if (SOCKET_ERROR == retval) return;
	bufSize += retval;
	this->Reconstruct(s);
}

void MsgReconstructor::Reconstruct(SOCKET s)
{
	char* curPos = buf.data();
	while (bufSize > 0) {
		if (preRemainSize > 0) {
			if (bufSize >= preRemainSize) {
				memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, preRemainSize);
				msgHandler(s, *reinterpret_cast<MsgBase*>(backBuf.data()));
				bufSize -= preRemainSize; curPos += preRemainSize;
				backBufSize = 0; preRemainSize = 0;
			}
			else {
				memcpy_s(backBuf.data() + backBufSize, backBufMaxLen - backBufSize, curPos, bufSize);
				backBufSize += bufSize; preRemainSize -= bufSize;
				bufSize = 0;
				return;
			}
		}
		else if (bufSize < sizeof(short)) {
			if (curPos != buf.data()) {
				memcpy_s(buf.data(), bufMaxLen, curPos, bufSize);
			}
			return;
		}
		else {
			short packetSize = *reinterpret_cast<short*>(curPos);
			if (packetSize > bufSize) {
				if (backBufMaxLen < packetSize) {
					backBuf.reserve(packetSize);
					backBufMaxLen = packetSize;
				}
				memcpy_s(backBuf.data(), backBufMaxLen, curPos, bufSize);
				preRemainSize = packetSize - bufSize;
				bufSize = 0;
				return;
			}
			else { 
				msgHandler(s, *reinterpret_cast<MsgBase*>(curPos));
				bufSize -= packetSize; curPos += packetSize;
			}
		}
	}
}