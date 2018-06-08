#pragma once
#include "../Share/Shares.h"

struct Client;

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

class NetworkManager {
public:
	void SendNetworkMessage(int id, MsgBase& msg);
	void SendNetworkMessage(SOCKET sock, MsgBase& msg);
	void SendNetworkMessage(std::vector<SOCKET>& sockList, MsgBase& msg);
	void RecvNetworkMessage(Client& sock);
private:
	void Send(ExtOverlapped& eov);
	void Recv(ExtOverlapped& eov);
};

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);