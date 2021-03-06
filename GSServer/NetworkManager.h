#pragma once
#include "../Share/Shares.h"
#include "Event.h"
#include "typedef.h"

struct ExtOverlappedBase {
	WSAOVERLAPPED ov;
	bool isNetworkEvent;

	ExtOverlappedBase(bool isNetwork) : isNetworkEvent{ isNetwork } {}
};

struct ExtOverlappedNetwork : public ExtOverlappedBase {
	SOCKET s;
	Client* client;
	std::unique_ptr<MsgBase> msg; // 동일한 메시지를 Broadcasting하는 경우, shared_ptr로 공유해서 사용
	bool isRecv{ false };

	ExtOverlappedNetwork(SOCKET s, MsgBase& msg) : ExtOverlappedBase{ true }, s{ s }, client{ nullptr }, msg{ &msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlappedNetwork(SOCKET s, std::unique_ptr<MsgBase>&& msg) : ExtOverlappedBase{ true }, s{ s }, client{ nullptr }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlappedNetwork(Client& client);

	ExtOverlappedNetwork(const ExtOverlappedNetwork&) = delete;
	ExtOverlappedNetwork& operator=(const ExtOverlappedNetwork&) = delete;
};

struct ExtOverlappedEvent : public ExtOverlappedBase {
	std::unique_ptr<const EventBase> msg;

	ExtOverlappedEvent(const EventBase& msg);
	ExtOverlappedEvent(std::unique_ptr<const EventBase>&& msg);
	ExtOverlappedEvent(const ExtOverlappedEvent&) = delete;
	ExtOverlappedEvent& operator=(const ExtOverlappedEvent&) = delete;
};

class NetworkManager {
public:
	void SendNetworkMessageWithID(int id, MsgBase& msg, ObjectMap& map);
	void SendNetworkMessage(SOCKET sock, MsgBase& msg);
	void RecvNetworkMessage(Client& sock);
private:
	void Send(ExtOverlappedNetwork& eov);
	void Recv(ExtOverlappedNetwork& eov);
};

struct ServerMsgHandler {
	Client* client;

	ServerMsgHandler() : client{ nullptr } {}
	ServerMsgHandler(Client& c) : client{ &c } {}
	void operator()(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

void SendCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov);
