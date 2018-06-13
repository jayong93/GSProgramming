#include "stdafx.h"
#include "ObjectManager.h"
#include "AIQueue.h"
#include "NetworkManager.h"
#include "LuaFunctionCall.h"
#include "Globals.h"

void NetworkManager::SendNetworkMessageWithID(int id, MsgBase & msg)
{
	auto locked = objManager.GetUniqueCollection();

	auto it = locked->find(id);
	if (it == locked->end()) return;
	auto& client = *reinterpret_cast<Client*>(it->second.get());

	auto eov = new ExtOverlapped{ client.s, msg };
	this->Send(*eov);
}

void NetworkManager::SendNetworkMessage(SOCKET sock, MsgBase & msg)
{
	auto eov = new ExtOverlapped{ sock, msg };
	this->Send(*eov);
}

void NetworkManager::RecvNetworkMessage(Client& c)
{
	auto eov = new ExtOverlapped{ c.s, c };
	this->Recv(*eov);
}

void NetworkManager::Send(ExtOverlapped & eov)
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

void NetworkManager::Recv(ExtOverlapped & eov)
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

		auto nearList = objManager.GetNearList(client->id);
		client->UpdateViewList(nearList);
		for (auto& id : nearList) {
			if (objManager.IsPlayer(id)) continue;

			UniqueLocked<lua_State*> lock;
			{
				auto locked = objManager.GetUniqueCollection();
				lock = ((AI_NPC*)locked->at(id).get())->GetLuaState();
			}

			LFCPlayerMoved{ client->id, client->x, client->y }(lock);
		}
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

		auto nearList = objManager.GetNearList(client->id);
		client->UpdateViewList(nearList);
	}
	break;
	}
}

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		if (0 != error) print_network_error(error);
		RemoveClient(eov.client);
		return;
	}
}

void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		if (0 != error) print_network_error(error);
		RemoveClient(eov.client);
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	eov.client->msgRecon.AddSize(transferred);
	eov.client->msgRecon.Reconstruct(eov.s);

	networkManager.RecvNetworkMessage(*eov.client);
}

ExtOverlappedNPC::ExtOverlappedNPC(const NPCMsg & msg) : msg{ &msg } { ZeroMemory(&ov, sizeof(ov)); }

ExtOverlappedNPC::ExtOverlappedNPC(std::unique_ptr<const NPCMsg>&& msg) : msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
