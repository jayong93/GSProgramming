#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "Globals.h"

void NetworkManager::SendNetworkMessage(int id, MsgBase & msg)
{
	auto locked = objManager.GetSharedCollection();
	auto& objMap = locked.data;

	auto it = objMap.find(id);
	if (it == objMap.end()) return;
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
			std::unique_lock<std::shared_timed_mutex> lg{ client->lock };
			client->x = newClientX;
			client->y = newClientY;
		}

		sectorManager.UpdateSector(client->id, oldX, oldY, newClientX, newClientY);
		networkManager.SendNetworkMessage(client->s, *new MsgMoveObject{ client->id, client->x, client->y });

		client->UpdateViewList();
		printf_s("client #%d has moved\n", client->id);
	}
	break;
	}
}

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		RemoveClient(eov.client);
		return;
	}
}

void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		RemoveClient(eov.client);
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	eov.client->msgRecon.AddSize(transferred);
	eov.client->msgRecon.Reconstruct(eov.s);

	networkManager.RecvNetworkMessage(*eov.client);
}

void NPCMsgCallback(DWORD error, ExtOverlappedNPC *& ov)
{
	using namespace std::chrono;
	auto recvTime = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
	switch (ov->msg.type) {
	case NpcMsgType::MOVE_RANDOM:
	{
		auto locked = objManager.GetSharedCollection();
		auto& npcMap = locked.data;
		const auto it = npcMap.find(ov->msg.id);
		if (it == npcMap.end()) break;
		auto& npc = it->second;

		const auto direction = rand() % 4;
		auto newX = npc->x;
		auto newY = npc->y;
		{
			std::unique_lock<std::shared_timed_mutex> lg{ npc->lock };
			switch (direction) {
			case 0: // 왼쪽
				newX -= 1;
				break;
			case 1: // 오른쪽
				newX += 1;
				break;
			case 2: // 위
				newY -= 1;
				break;
			case 3: // 아래
				newY += 1;
				break;
			}
			npc->x = max(0, min(newX, BOARD_W - 1));
			npc->y = max(0, min(newY, BOARD_H - 1));
		}

		npc->UpdateViewList();

		std::shared_lock<std::shared_timed_mutex> lg{ npc->lock };
		if (npc->viewList.size() > 0) {
			npcMsgQueue.Push(NPCMsg(npc->id, NpcMsgType::MOVE_RANDOM, recvTime + 1000));
		}
	}
	break;
	}
}
