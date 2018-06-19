#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "LuaFunctionCall.h"
#include "NPC.h"
#include "Globals.h"
#include <cassert>

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

void NetworkManager::RecvNetworkMessage(Client& c)
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
	assert(eov.client != nullptr && "The recv eov has no client");
	auto& msgRecon = eov.client->GetMessageConstructor();
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

void ServerMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	if (nullptr == client) return;
	auto rType = (MsgTypeCS)msg.type;
	switch (rType) {
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
		client->Move(dx, dy);
		const auto[newX, newY] = client->GetPos();
		const auto id = client->GetID();

		networkManager.SendNetworkMessage(client->GetSocket(), *new MsgSetPosition( id, newX, newY ));

		objManager.Access([client{ this->client }, clientId{ id }](auto& map) {
			UpdateViewList(clientId, map);
		});
	}
	break;
	case MsgTypeCS::TELEPORT:
	{
		auto& rMsg = *(const MsgTeleport*)&msg;
		client->SetPos(rMsg.x, rMsg.y);
		const auto[newX, newY] = client->GetPos();
		const auto id = client->GetID();

		networkManager.SendNetworkMessage(client->GetSocket(), *new MsgSetPosition( id, newX, newY ));

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
		RemoveClient(ov->client);
		return;
	}
}

void RecvCompletionCallback(DWORD error, DWORD transferred, std::unique_ptr<ExtOverlappedNetwork>& ov)
{
	if (0 != error || 0 == transferred) {
		if (0 != error) print_network_error(error);
		RemoveClient(ov->client);
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	auto& msgRecon = ov->client->GetMessageConstructor();
	msgRecon.AddSize(transferred);
	msgRecon.Reconstruct(ov->s);

	networkManager.RecvNetworkMessage(*ov->client);
}


ExtOverlappedEvent::ExtOverlappedEvent(const EventBase & msg) : ExtOverlappedBase{ false }, msg{ &msg } {}

ExtOverlappedEvent::ExtOverlappedEvent(std::unique_ptr<const EventBase>&& msg) : ExtOverlappedBase{ false }, msg{ std::move(msg) } {}

ExtOverlappedNetwork::ExtOverlappedNetwork(Client & client) : ExtOverlappedBase{ true }, s{ client.GetSocket() }, client{ &client } {}
