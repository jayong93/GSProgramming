#pragma once
#include "../Share/Shares.h"

constexpr int CELL_W = 40, CELL_H = 40;

struct Player {
	short x, y;
	COLORREF color;

	Player() {}
	Player(short x, short y, Color color) : x{ x }, y{ y }, color{RGB(color.r, color.g, color.b)} {}
};

struct ClientMsgHandler {
	void operator()(SOCKET s, const MsgBase& msg);
	~ClientMsgHandler() {}
};

bool InitNetwork();
void SendMovePacket(SOCKET sock, UINT_PTR key);
int SendAll(SOCKET sock, const char* buf, size_t len);
void DrawPlayer(HDC memDC, const int leftTopCoordX, const int leftTopCoordY, const Player& p);
void ErrorQuit();
void RecvThreadFunc();
