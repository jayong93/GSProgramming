#pragma once
#include "../Share/Shares.h"

struct Client;

constexpr int CELL_W = 40, CELL_H = 40;
constexpr int MAX_DUMMY_NUM = 500;
constexpr int DRAW_OFFSET = 20;
constexpr int DUMMY_RADIUS = 5;

struct ClientMsgHandler {
	Client* client;

	ClientMsgHandler(Client& client) : client{ &client } {}
	~ClientMsgHandler() {}

	void operator()(SOCKET s, const MsgBase& msg);
};

bool InitNetwork();
void InitDummyClients(int client_num);
void DrawPlayer(HDC memDC, const int x, const int y);
void ErrorQuit();
void WorkerThreadFunc();
void RemoveClient(Client& c);
