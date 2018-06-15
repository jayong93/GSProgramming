#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "LuaFunctionCall.h"
#include "Globals.h"

void NetworkManager::SendNetworkMessageWithID(int id, MsgBase & msg, ObjectMap& map)
{
	auto it = map.find(id);
	if (it == map.end()) return;
	auto& client = *reinterpret_cast<Client*>(it->second.get());

	auto eov = new ExtOverlappedNetwork{ client.s, msg };
	this->Send(*eov);
}

void NetworkManager::SendNetworkMessage(SOCKET sock, MsgBase & msg)
{
	auto eov = new ExtOverlappedNetwork{ sock, msg };
	this->Send(*eov);
}

void NetworkManager::RecvNetworkMessage(Client& c)
{
	auto eov = new ExtOverlappedNetwork{ c.s, c };
	this->Recv(*eov);
}

void NetworkManager::Send(ExtOverlappedNetwork & eov)
{
	if (!eov.msg) return;
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
	if (eov.client == nullptr) return;
	WSABUF wb;
	wb.buf = eov.client->msgRecon.GetBuffer();
	wb.len = eov.client->msgRecon.GetSize();
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
	switch (msg.type) {
	case MsgType::CS_INPUT_MOVE:
	{
		auto& rMsg = *(const MsgInputMove*)(&msg);
		auto oldX = client->x;
		auto oldY = client->y;
		{
			std::unique_lock<std::mutex> lg{ client->lock };
			client->Move(rMsg.dx, rMsg.dy);
		}

		sectorManager.UpdateSector(client->id, oldX, oldY, client->x, client->y);
		networkManager.SendNetworkMessage(client->s, *new MsgMoveObject{ client->id, client->x, client->y });

		objManager.LockAndExec([this](auto& map) {
			UpdateViewList(this->client->id, map);
			auto nearList = objManager.GetNearList(this->client->id, map);
			for (auto& id : nearList) {
				if (ObjectManager::IsPlayer(id)) continue;
				auto it = map.find(id);
				if (map.end() == it) continue;
				auto& npc = *reinterpret_cast<AI_NPC*>(it->second.get());

				LuaCall call( "player_moved", {(long long)this->client->id, (long long)this->client->x, (long long)this->client->y}, 0 );
				npc.lua.Call(call, npc, map);
			}
		});
	}
	break;
	case MsgType::CS_TELEPORT:
	{
		auto& rMsg = *(const MsgTeleport*)&msg;
		auto oldX = client->x;
		auto oldY = client->y;
		{
			std::unique_lock<std::mutex> lg{ client->lock };
			client->x = rMsg.x;
			client->y = rMsg.y;
		}

		sectorManager.UpdateSector(client->id, oldX, oldY, client->x, client->x);
		networkManager.SendNetworkMessage(client->s, *new MsgMoveObject{ client->id, client->x, client->y });

		auto id = this->client->id;
		objManager.LockAndExec([id](auto& map) {
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
	ov->client->msgRecon.AddSize(transferred);
	ov->client->msgRecon.Reconstruct(ov->s);

	networkManager.RecvNetworkMessage(*ov->client);
}


ExtOverlappedEvent::ExtOverlappedEvent(const EventBase & msg) : ExtOverlappedBase{ false }, msg{ &msg } { ZeroMemory(&ov, sizeof(ov)); }

ExtOverlappedEvent::ExtOverlappedEvent(std::unique_ptr<const EventBase>&& msg) : ExtOverlappedBase{ false }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
