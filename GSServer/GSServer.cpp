#pragma comment(lib, "ws2_32")

#include "stdafx.h"
#include "GSServer.h"

MsgReconstructor msgRecon{ 100, MessageHandler };
char cx, cy;

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

	int retval;
	while (true) {
		auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);
		if (clientSock == INVALID_SOCKET) err_quit_wsa(TEXT("WSAAccept"));
		printf_s("client has connected\n");
		auto eov = new ExtOverlapped(clientSock, msgRecon);
		if (0 < (retval = OverlappedRecv(*eov))) err_quit_wsa(retval, TEXT("OverlappedRecv"));
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
}

void MessageHandler(SOCKET s, const MsgBase & msg)
{
	switch (msg.type) {
	case MsgType::INPUT_MOVE:
		{
			auto& rMsg = *(const MsgInputMove*)(&msg);
			cx = max(0, min(cx + rMsg.dx, BOARD_W-1));
			cy = max(0, min(cy + rMsg.dy, BOARD_H-1));

			auto ov = new ExtOverlapped{ s, std::shared_ptr<MsgBase>{new MsgMoveCharacter{ cx, cy }} };
			OverlappedSend(*ov);
		}
		break;
	}
}

int OverlappedRecv(ExtOverlapped & ov)
{
	if (nullptr == ov.msgRecon) return -1;
	WSABUF wb;
	wb.buf = ov.msgRecon->GetBuffer();
	wb.len = ov.msgRecon->GetSize();
	DWORD flags{ 0 };
	const int retval = WSARecv(ov.s, &wb, 1, nullptr, &flags, (LPWSAOVERLAPPED)&ov, RecvCompletionCallback);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return 0;
	return error;
}

int OverlappedSend(ExtOverlapped & ov)
{
	if (!ov.msg) return -1;
	WSABUF wb;
	wb.buf = (char*)ov.msg.get();
	wb.len = ov.msg->len;
	const int retval = WSASend(ov.s, &wb, 1, nullptr, 0, (LPWSAOVERLAPPED)&ov, SendCompletionCallback);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return 0;
	return error;
}

void SendCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		closesocket(eov.s);
	}
	delete &eov;
}

void RecvCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		closesocket(eov.s);
	}
	eov.msgRecon->AddSize(transferred);
	eov.msgRecon->Reconstruct(eov.s);
	auto nov = new ExtOverlapped{ eov.s, msgRecon };
	int retval;
	if (0 < (retval = OverlappedRecv(*nov))) err_quit_wsa(retval, TEXT("OverlappedRecv"));
	delete &eov;
}
