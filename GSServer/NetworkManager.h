#pragma once
#include "../Share/Shares.h"
#include "typedef.h"

class EventBase;

struct ExtOverlappedBase {
	WSAOVERLAPPED ov;
	bool isNetworkEvent;

	ExtOverlappedBase(bool isNetwork) : isNetworkEvent{ isNetwork } { ZeroMemory(&ov, sizeof(ov)); }
};

struct ExtOverlappedNetwork : public ExtOverlappedBase {
	SOCKET s;
	MessageReceiver* receiver;
	std::unique_ptr<MsgBase> msg;
	bool isRecv{ false };

	ExtOverlappedNetwork(SOCKET s, MsgBase& msg) : ExtOverlappedBase{ true }, s{ s }, receiver{ nullptr }, msg{ &msg } {}
	ExtOverlappedNetwork(SOCKET s, std::unique_ptr<MsgBase>&& msg) : ExtOverlappedBase{ true }, s{ s }, receiver{ nullptr }, msg{ std::move(msg) } {}
	ExtOverlappedNetwork(MessageReceiver& client);

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
	void RecvNetworkMessage(MessageReceiver& sock);
private:
	void Send(ExtOverlappedNetwork& eov);
	void Recv(ExtOverlappedNetwork& eov);
};

struct ServerMsgHandler {
	MessageReceiver& receiver;

	ServerMsgHandler(MessageReceiver& receiver);
	void operator()(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

struct MessageReceiver {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	Client* owner;

	MessageReceiver(SOCKET s, size_t size, const ServerMsgHandler& handler);
	MessageReceiver(SOCKET s, size_t size = 100);
};

void SendCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov);
