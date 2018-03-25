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
void MessageHandler(const MsgBase& msg);
bool OverlappedRecv(ExtOverlapped& ov);
bool OverlappedSend(ExtOverlapped& ov);
void CALLBACK CompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag);