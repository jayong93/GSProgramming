#pragma once
#include "../Share/Shares.h"
#include "AIQueue.h"

class Client;

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	SOCKET s;
	Client* client;
	std::unique_ptr<MsgBase> msg; // 동일한 메시지를 Broadcasting하는 경우, shared_ptr로 공유해서 사용
	bool isRecv{ false };

	ExtOverlapped(SOCKET s, MsgBase& msg) : s{ s }, client{ nullptr }, msg{ &msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, std::unique_ptr<MsgBase>&& msg) : s{ s }, client{ nullptr }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, Client& client) : s{ s }, client{ &client } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

struct ExtOverlappedNPC {
	WSAOVERLAPPED ov;
	NPCMsg msg;

	ExtOverlappedNPC(const NPCMsg& msg) : msg{ msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlappedNPC(const ExtOverlappedNPC&) = delete;
	ExtOverlappedNPC& operator=(const ExtOverlappedNPC&) = delete;
};

class NetworkManager {
public:
	void SendNetworkMessage(int id, MsgBase& msg);
	void SendNetworkMessage(SOCKET sock, MsgBase& msg);
	void RecvNetworkMessage(Client& sock);
private:
	void Send(ExtOverlapped& eov);
	void Recv(ExtOverlapped& eov);
};

struct ServerMsgHandler {
	Client* client;

	ServerMsgHandler() : client{ nullptr } {}
	ServerMsgHandler(Client& c) : client{ &c } {}
	void operator()(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void NPCMsgCallback(DWORD error, ExtOverlappedNPC*& ov);