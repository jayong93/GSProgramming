#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "LuaFunctionCall.h"
#include "NPC.h"
#include "Globals.h"
#include "GSServer.h"
#include <cassert>

static std::unordered_set<std::wstring> loginedNames;

void NetworkManager::SendNetworkMessageWithID(int id, MsgBase & msg, ObjectMap& map)
{
	auto it = map.find(id);
	if (it == map.end()) return;
	auto& client = *reinterpret_cast<Client*>(it->second.get());

	auto eov = new ExtOverlappedNetwork{ client.GetSocket(), msg };
	this->Send(*eov);
}

void NetworkManager::SendNetworkMessage(SOCKET sock, MsgBase & msg)
{
	auto eov = new ExtOverlappedNetwork{ sock, msg };
	this->Send(*eov);
}

void NetworkManager::RecvNetworkMessage(MessageReceiver& c)
{
	auto eov = new ExtOverlappedNetwork{ c };
	this->Recv(*eov);
}

void NetworkManager::Send(ExtOverlappedNetwork & eov)
{
	assert(eov.msg && "The send eov has no msg");
	WSABUF wb;
	wb.buf = (char*)eov.msg.get();
	wb.len = eov.msg->len;
	eov.isRecv = false;
	const int retval = WSASend(eov.s, &wb, 1, nullptr, 0, (LPWSAOVERLAPPED)&eov, nullptr);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return;
	if (error > 0) print_network_error(error);
}

void NetworkManager::Recv(ExtOverlappedNetwork & eov)
{
	assert(eov.receiver != nullptr && "The recv eov has no receiver");
	auto& msgRecon = eov.receiver->msgRecon;
	WSABUF wb;
	wb.buf = msgRecon.GetBuffer();
	wb.len = msgRecon.GetSize();
	eov.isRecv = true;
	DWORD flags{ 0 };
	const int retval = WSARecv(eov.s, &wb, 1, nullptr, &flags, (LPWSAOVERLAPPED)&eov, nullptr);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return;
	if (error > 0) print_network_error(error);
}

ServerMsgHandler::ServerMsgHandler(MessageReceiver & receiver) : receiver{ receiver } {}

void ServerMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	auto rType = (MsgTypeCS)msg.type;
	switch (rType) {
	case MsgTypeCS::LOGIN:
	{
		auto& rMsg{ (MsgLogin&)msg };
		auto result = [s, &receiver{ this->receiver }](bool result, const DBData& data) {
			const auto&[name, xPos, yPos, lv, hp, exp] = data;

			if (result && loginedNames.insert(name).second) {
				AddNewClient(s, name.c_str(), xPos, yPos, receiver);
			}
			else if (!result) {
				loginedNames.insert(name);
				WORD newX, newY;
				newX = posRange(rndGen); newY = posRange(rndGen);
				auto newClientData = AddNewClient(s, name.c_str(), newX, newY, receiver);
				if (newClientData) {
					dbMsgQueue.Push(new DBAddUser{ hstmt, *newClientData });
				}
			}
			else {
				networkManager.SendNetworkMessage(s, *new MsgLoginFail);
			}
		};

		dbMsgQueue.Push(MakeGetUserDataQuery(hstmt, rMsg.gameID, result));
	}
	break;
	case MsgTypeCS::MOVE:
	{
		auto& rMsg = (MsgInputMove&)msg;
		short dx{ 0 }, dy{ 0 };
		switch (rMsg.direction) {
		case 0: // UP
			dy = -1;
			break;
		case 1: // DOWN
			dy = 1;
			break;
		case 2: //LEFT
			dx = -1;
			break;
		case 3: // RIGHT
			dx = 1;
			break;
		default:
			return;
		}
		auto& client = receiver.owner;
		client->Move(dx, dy);
		const auto[newX, newY] = client->GetPos();
		const auto id = client->GetID();

		networkManager.SendNetworkMessage(client->GetSocket(), *new MsgSetPosition(id, newX, newY));

		objManager.Access([client, clientId{ id }](auto& map) {
			UpdateViewList(clientId, map);
		});
	}
	break;
	case MsgTypeCS::TELEPORT:
	{
		auto& rMsg = *(const MsgTeleport*)&msg;
		auto& client = receiver.owner;
		client->SetPos(rMsg.x, rMsg.y);
		const auto[newX, newY] = client->GetPos();
		const auto id = client->GetID();

		networkManager.SendNetworkMessage(client->GetSocket(), *new MsgSetPosition(id, newX, newY));

		objManager.Access([id](auto& map) {
			UpdateViewList(id, map);
		});
	}
	break;
	}
}

void SendCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov)
{
	if (0 != error || 0 == transferred) {
		if (0 != error) print_network_error(error);
		RemoveClient(ov->receiver->owner);
		return;
	}
}

void RecvCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov)
{
	if (0 != error || 0 == transferred) {
		if (0 != error) print_network_error(error);
		RemoveClient(ov->receiver->owner);
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	auto& msgRecon = ov->receiver->msgRecon;
	msgRecon.AddSize(transferred);
	msgRecon.Reconstruct(ov->s);

	networkManager.RecvNetworkMessage(*ov->receiver);
}


ExtOverlappedEvent::ExtOverlappedEvent(const EventBase & msg) : ExtOverlappedBase{ false }, msg{ &msg } {}

ExtOverlappedEvent::ExtOverlappedEvent(std::unique_ptr<const EventBase>&& msg) : ExtOverlappedBase{ false }, msg{ std::move(msg) } {}

ExtOverlappedNetwork::ExtOverlappedNetwork(MessageReceiver & receiver) : ExtOverlappedBase{ true }, s{ receiver.s }, receiver{ &receiver } {}

MessageReceiver::MessageReceiver(SOCKET s, size_t size, const ServerMsgHandler & handler) : s{ s }, msgRecon { size, handler } {}

MessageReceiver::MessageReceiver(SOCKET s, size_t size) : MessageReceiver{ s, size, ServerMsgHandler{*this} } {}
