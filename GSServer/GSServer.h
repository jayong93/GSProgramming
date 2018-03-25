#pragma once
#include "../Share/Shares.h"

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	SOCKET s;
	MsgReconstructor* msgRecon;
	std::shared_ptr<MsgBase> msg;

	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>& msg) : s{ s }, msgRecon{ nullptr }, msg { msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>&& msg) : s{ s }, msg { std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, MsgReconstructor& msgRecon) : s{ s }, msgRecon{ &msgRecon } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

void SendMovePacket(SOCKET sock, char* buf, int x, int y);
void MessageHandler(SOCKET s, const MsgBase& msg);
int OverlappedRecv(ExtOverlapped& ov);
int OverlappedSend(ExtOverlapped& ov);
void CALLBACK SendCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag);
void CALLBACK RecvCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag);