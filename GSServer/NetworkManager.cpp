#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "Globals.h"

void NetworkManager::SendNetworkMessage(int id, MsgBase & msg, ObjectMap& map)
{
	auto it = map.find(id);
	if (it == map.end()) return;
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
		auto newClientX = max(0, min(client->x + rMsg.dx, BOARD_W - 1));
		auto newClientY = max(0, min(client->y + rMsg.dy, BOARD_H - 1));
		auto oldX = client->x;
		auto oldY = client->y;
		{
			std::unique_lock<std::mutex> lg{ client->lock };
			client->x = newClientX;
			client->y = newClientY;
		}

		sectorManager.UpdateSector(client->id, oldX, oldY, client->x, client->y);
		networkManager.SendNetworkMessage(client->s, *new MsgMoveObject{ client->id, client->x, client->y });

		auto nearList = objManager.GetNearList(client->id);
		objManager.LockAndExec([this, &nearList](auto& map) {UpdateViewList(this->client->id, nearList, map); });
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
		objManager.LockAndExec([this, &nearList](auto& map) {UpdateViewList(this->client->id, nearList, map); });
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

