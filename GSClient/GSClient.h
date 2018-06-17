#pragma once
#include "../Share/Shares.h"
#include "typedef.h"

constexpr int CELL_W = 40, CELL_H = 40;

struct ClientMsgHandler {
	Client* client;

	ClientMsgHandler(Client& client) : client{ &client } {}
	~ClientMsgHandler() {}

	void operator()(SOCKET s, const MsgBase& msg);
};

bool InitNetwork();
void SendMovePacket(SOCKET sock, UINT_PTR key);
void DrawObject(HDC memDC, const int leftTopCoordX, const int leftTopCoordY, Object & p);
void DrawChat(HDC memDC, const int leftTopX, const int leftTopY, Object& obj);
void DrawHP(HDC memDC, const int leftTopX, const int leftTopY, Object& obj);
void ErrorQuit();
void WorkerThreadFunc();
