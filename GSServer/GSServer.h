#pragma once
#include "../Share/Shares.h"

struct Client {
	unsigned int id;
	MsgReconstructor msgRecon;
	SOCKET s;
	Color color;
	char x, y;

	Client() : msgRecon{ 0, nullptr }, color{ 0,0,0 } {}
	Client(unsigned int id, SOCKET s, Color c, char x, char y, std::function<void(SOCKET, const MsgBase&, void*)> handler) : id{ id }, msgRecon{ 100, handler }, s{ s }, color{ c }, x{ x }, y{ y } {}
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
	std::shared_ptr<MsgBase> msg;

	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>& msg) : s{ s }, client{ nullptr }, msg{ msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>&& msg) : s{ s }, client{ nullptr }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, Client& client) : s{ s }, client{ &client } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

void MessageHandler(SOCKET s, const MsgBase& msg, void* ov);
int OverlappedRecv(ExtOverlapped& ov);
int OverlappedSend(ExtOverlapped& ov);
void CALLBACK SendCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag);
void CALLBACK RecvCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag);
void RemoveClient(Client& client);