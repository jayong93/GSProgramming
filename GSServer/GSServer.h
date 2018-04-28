#pragma once
#include "../Share/Shares.h"

using Sector = std::unordered_set<int>;
struct Client;

struct ServerMsgHandler : public MsgHandler {
	Client& client;

	ServerMsgHandler(Client& c) : client{ c } {}
	virtual void ProcessMessage(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

struct Client {
	std::shared_timed_mutex lock;
	unsigned int id;
	MsgReconstructor msgRecon;
	SOCKET s;
	Color color;
	char x, y;

	Client() : msgRecon{}, color{ 0,0,0 } {}
	Client(unsigned int id, SOCKET s, Color c, char x, char y) : id{ id }, msgRecon{ 100, *new ServerMsgHandler{*this} }, s{ s }, color{ c }, x{ x }, y{ y } {}
	Client(Client&& o) : id{ o.id }, msgRecon{ std::move(o.msgRecon) }, s{ o.s }, color{ o.color }, x{ o.x }, y{ o.y } {}
	Client& operator=(Client&& o) {
		id = o.id;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
		color = o.color;
		x = o.x;
		y = o.y;
	}
};

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	SOCKET s;
	Client* client;
	std::shared_ptr<MsgBase> msg; // 동일한 메시지를 Broadcasting하는 경우, shared_ptr로 공유해서 사용
	bool isRecv{ false };

	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>& msg) : s{ s }, client{ nullptr }, msg{ msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>&& msg) : s{ s }, client{ nullptr }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, Client& client) : s{ s }, client{ &client } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

int OverlappedRecv(ExtOverlapped& ov);
int OverlappedSend(ExtOverlapped& ov);
void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void RemoveClient(Client& client);
void AcceptThreadFunc();
void WorkerThreadFunc();