#pragma once
#include "../Share/Shares.h"

using Sector = std::unordered_set<unsigned int>;
struct Client;

struct ServerMsgHandler {
	Client* client;

	ServerMsgHandler() : client{ nullptr } {}
	ServerMsgHandler(Client& c) : client{ &c } {}
	void operator()(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

struct Client {
	std::shared_timed_mutex lock;
	unsigned int id;
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	Color color;
	short x, y;
	std::unordered_set<unsigned int> viewList;

	Client() : msgRecon{}, color{ 0,0,0 } {}
	Client(unsigned int id, SOCKET s, Color c, short x, short y) : id{ id }, msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, color{ c }, x{ x }, y{ y } {}
	Client(Client&& o) : id{ o.id }, msgRecon{ std::move(o.msgRecon) }, s{ o.s }, color{ o.color }, x{ o.x }, y{ o.y }, viewList{ std::move(o.viewList) } {}
	Client& operator=(Client&& o) {
		id = o.id;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
		color = o.color;
		x = o.x;
		y = o.y;
		viewList = std::move(o.viewList);
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
unsigned int PositionToSectorIndex(unsigned int x, unsigned int y);
std::vector<Sector*> GetNearSectors(unsigned int sectorIdx);
std::unordered_set<unsigned int> GetNearList(Client& c);
void UpdateViewList(Client& c);