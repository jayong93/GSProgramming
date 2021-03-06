#pragma once
#include "../Share/Shares.h"

struct Object;
struct Client;

constexpr int CELL_W = 40, CELL_H = 40;

struct ClientMsgHandler {
	Client* client;

	ClientMsgHandler(Client& client) : client{ &client } {}
	~ClientMsgHandler() {}

	void operator()(SOCKET s, const MsgBase& msg);
};

bool InitNetwork();
void SendMovePacket(SOCKET sock, UINT_PTR key);
void DrawPlayer(HDC memDC, const int leftTopCoordX, const int leftTopCoordY, const Object & p);
void ErrorQuit();
void WorkerThreadFunc();
void RemoveClient(Client& c);
