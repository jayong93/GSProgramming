#pragma once
#include "../Share/Shares.h"

struct Client;

constexpr int CELL_W = 40, CELL_H = 40;
constexpr int MAX_DUMMY_NUM = 500;

struct ClientMsgHandler {
	Client& client;

	ClientMsgHandler(Client& client) : client{ client } {}
	~ClientMsgHandler() {}

	void operator()(SOCKET s, const MsgBase& msg);
};

bool InitNetwork();
void InitDummyClients(int client_num);
void SendMovePacket(SOCKET sock, UINT_PTR key);
int SendAll(SOCKET sock, const char* buf, size_t len);
void DrawPlayer(HDC memDC, const int x, const int y);
void ErrorQuit();
void WorkerThreadFunc();
void RemoveClient(Client& c);
