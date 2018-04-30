#pragma once
#include "../Share/Shares.h"

constexpr int CELL_W = 50, CELL_H = 50;

struct Player {
	char x, y;
	COLORREF color;

	Player() {}
	Player(char x, char y, Color color) : x{ x }, y{ y }, color{RGB(color.r, color.g, color.b)} {}
};

struct ClientMsgHandler : public MsgHandler {
	virtual void ProcessMessage(SOCKET s, const MsgBase& msg);
	~ClientMsgHandler() {}
};

bool InitNetwork();
void NetworkHandler();
void SendMovePacket(SOCKET sock, UINT_PTR key);
int SendAll(SOCKET sock, const char* buf, size_t len);
void ErrorQuit();
