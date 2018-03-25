#pragma comment(lib, "ws2_32")

#include "stdafx.h"
#include "GSServer.h"

MsgReconstructor msgRecon{ 100, MessageHandler };

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	auto sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
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

	WSABUF recvBufs;
	recvBufs.buf = msgRecon.GetBuffer();
	recvBufs.len = msgRecon.GetSize();

	while (true) {
		auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);
		if (clientSock == INVALID_SOCKET) err_quit_wsa(TEXT("WSAAccept"));
		//u_long isNonBlock{ TRUE };
		//ioctlsocket(clientSock, FIONBIO, &isNonBlock);
		printf_s("client has connected\n");
		auto eov = new ExtOverlapped(clientSock, msgRecon);
		OverlappedRecv(*eov);
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
}

void SendMovePacket(SOCKET sock, char* buf, int x, int y) {
}

void MessageHandler(const MsgBase & msg)
{
}

bool OverlappedRecv(ExtOverlapped & ov)
{
	if (nullptr == ov.msgRecon) return false;
	WSABUF wb;
	wb.buf = ov.msgRecon->GetBuffer();
	wb.len = ov.msgRecon->GetSize();
	const int retval = WSARecv(ov.s, &wb, 1, nullptr, 0, (LPWSAOVERLAPPED)&ov, CompletionCallback);
	if (0 == retval || WSAGetLastError() == WSA_IO_PENDING) return true;
	return false;
}

bool OverlappedSend(ExtOverlapped & ov)
{
	return false;
}

void CompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag)
{
}
