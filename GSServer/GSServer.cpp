#pragma comment(lib, "ws2_32")

#include "stdafx.h"
#include "GSServer.h"

unsigned int nextId{ 0 };
std::unordered_map<unsigned int, std::unique_ptr<Client>> clientMap;
std::vector<Sector> sectorList;
std::shared_timed_mutex clientMapLock, sectorLock;
HANDLE iocpObject;

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	iocpObject = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	auto sectorNum = int(std::ceil((double)BOARD_W / (double)VIEW_SIZE) * std::ceil((double)BOARD_H / (double)VIEW_SIZE));
	sectorList.resize(sectorNum);

	std::vector<std::thread> threadList;
	threadList.emplace_back(AcceptThreadFunc);

	auto threadNum = std::thread::hardware_concurrency();
	if (0 == threadNum) threadNum = 1;
	for (auto i = 0; i < threadNum; ++i) { threadList.emplace_back(WorkerThreadFunc); }

	for (auto& t : threadList) { t.join(); }

	WSACleanup();
}

int OverlappedRecv(ExtOverlapped & ov)
{
	if (ov.client == nullptr) return -1;
	WSABUF wb;
	wb.buf = ov.client->msgRecon.GetBuffer();
	wb.len = ov.client->msgRecon.GetSize();
	ov.isRecv = true;
	DWORD flags{ 0 };
	const int retval = WSARecv(ov.s, &wb, 1, nullptr, &flags, (LPWSAOVERLAPPED)&ov, nullptr);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return 0;
	return error;
}

int OverlappedSend(ExtOverlapped & ov)
{
	if (!ov.msg) return -1;
	WSABUF wb;
	wb.buf = (char*)ov.msg.get();
	wb.len = ov.msg->len;
	ov.isRecv = false;
	const int retval = WSASend(ov.s, &wb, 1, nullptr, 0, (LPWSAOVERLAPPED)&ov, nullptr);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return 0;
	return error;
}

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		RemoveClient(*eov.client);
		return;
	}
	delete &eov;
}

void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		RemoveClient(*eov.client);
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	eov.client->msgRecon.AddSize(transferred);
	eov.client->msgRecon.Reconstruct(eov.s);

	auto nov = new ExtOverlapped{ eov.s, *eov.client };
	int retval;
	if (0 < (retval = OverlappedRecv(*nov))) err_quit_wsa(retval, TEXT("OverlappedRecv"));
	delete &eov;
}

void RemoveClient(Client & client)
{
	closesocket(client.s);
	std::unique_ptr<Client> localClient;
	{
		std::unique_lock<std::shared_timed_mutex> lg{ clientMapLock };
		auto it = clientMap.find(client.id);
		if (it != clientMap.end())
		{
			std::unique_lock<std::shared_timed_mutex> clg{ client.lock };
			localClient = std::move(it->second);
		}
		else return;
		clientMap.erase(it);
	}
	std::shared_ptr<MsgBase> msg{ new MsgRemoveObject{client.id} };
	{
		std::shared_lock<std::shared_timed_mutex> lg{ clientMapLock };
		for (auto& id : client.viewList) {
			auto it = clientMap.find(id);
			if (it == clientMap.end()) continue;
			auto& player = *it->second.get();
			std::unique_lock<std::shared_timed_mutex> plg{ player.lock };
			const auto removedCount = player.viewList.erase(client.id);
			if (removedCount == 1) {
				auto ov = new ExtOverlapped{ player.s, msg };
				OverlappedSend(*ov);
			}
		}
	}
}

void AcceptThreadFunc()
{
	auto sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	BOOL isNoDelay{ TRUE };
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&isNoDelay, sizeof(isNoDelay));

	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(GS_PORT);

	bind(sock, (const sockaddr*)&addr, sizeof(sockaddr));
	listen(sock, SOMAXCONN);
	sockaddr clientAddr;
	int addrLen = sizeof(clientAddr);

	std::random_device rd;
	std::mt19937_64 rndGen{ rd() };
	std::uniform_int_distribution<int> posRange{ 0, min(BOARD_W, BOARD_H) - 1 };
	std::uniform_int_distribution<int> colorRange{ 0, 255 };

	int retval;
	while (true) {
		auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);
		if (clientSock == INVALID_SOCKET) err_quit_wsa(TEXT("WSAAccept"));
		unsigned int clientId{ nextId++ };
		auto newClientPtr = std::make_unique<Client>(clientId, clientSock, Color(colorRange(rndGen), colorRange(rndGen), colorRange(rndGen)), posRange(rndGen), posRange(rndGen));
		Client& newClient = *newClientPtr.get();
		{
			std::unique_lock<std::shared_timed_mutex> lg{ clientMapLock };
			clientMap.emplace(clientId, std::move(newClientPtr));
		}
		const auto sectorIdx = PositionToSectorIndex(newClient.x, newClient.y);
		{
			std::unique_lock<std::shared_timed_mutex> lg{ sectorLock };
			sectorList[sectorIdx].emplace(newClient.id);
		}
		CreateIoCompletionPort((HANDLE)clientSock, iocpObject, clientId, 0);
		printf_s("client(id: %d) has connected\n", clientId);

		std::shared_ptr<MsgBase> giveIDMsg{ new MsgGiveID{clientId} };
		auto eov = new ExtOverlapped{ newClient.s, std::move(giveIDMsg) };
		if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

		std::shared_ptr<MsgBase> inMsg{ new MsgPutObject{ newClient.id, newClient.x, newClient.y, newClient.color } };
		eov = new ExtOverlapped{ newClient.s, std::move(inMsg) };
		if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

		UpdateViewList(newClient);

		eov = new ExtOverlapped(clientSock, newClient);
		if (0 < (retval = OverlappedRecv(*eov))) err_quit_wsa(retval, TEXT("OverlappedRecv"));
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
}

void WorkerThreadFunc()
{
	DWORD bytes;
	ULONG_PTR key;
	ExtOverlapped* ov;
	DWORD error{ 0 };
	while (true) {
		auto isSuccess = GetQueuedCompletionStatus(iocpObject, &bytes, &key, (LPOVERLAPPED*)&ov, INFINITE);
		if (FALSE == isSuccess) error = GetLastError();
		if (ov->isRecv) { RecvCompletionCallback(error, bytes, ov); }
		else { SendCompletionCallback(error, bytes, ov); }
	}
}

unsigned int PositionToSectorIndex(unsigned int x, unsigned int y)
{
	const auto sectorX = x / VIEW_SIZE;
	const auto sectorY = y / VIEW_SIZE;
	return sectorY * int(std::ceil(BOARD_W / (double)VIEW_SIZE)) + sectorX;
}

std::vector<Sector*> GetNearSectors(unsigned int sectorIdx)
{
	std::vector<Sector*> nearSectors;
	const auto horiSectorNum = int(std::ceil(BOARD_W / (double)VIEW_SIZE));
	const auto vertSectorNum = int(std::ceil(BOARD_H / (double)VIEW_SIZE));
	const int sectorX = sectorIdx % horiSectorNum;
	const int sectorY = sectorIdx / horiSectorNum;
	const auto sectorMinX = max(sectorX - 1, 0);
	const auto sectorMaxX = min(sectorX + 1, horiSectorNum - 1);
	const auto sectorMinY = max(sectorY - 1, 0);
	const auto sectorMaxY = min(sectorY + 1, vertSectorNum - 1);

	std::shared_lock<std::shared_timed_mutex> lg{ sectorLock };
	for (auto i = sectorMinY; i <= sectorMaxY; ++i) {
		for (auto j = sectorMinX; j <= sectorMaxX; ++j) {
			nearSectors.push_back(&sectorList[i*horiSectorNum + j]);
		}
	}
	return nearSectors;
}

std::unordered_set<unsigned int> GetNearList(Client & c)
{
	std::unordered_set<unsigned int> nearList;
	{
		auto nearSectors = GetNearSectors(PositionToSectorIndex(c.x, c.y));
		std::shared_lock<std::shared_timed_mutex> lg{ clientMapLock };
		for (auto s : nearSectors) {
			std::copy_if(s->begin(), s->end(), std::inserter(nearList, nearList.end()), [&](auto id) {
				if (id == c.id) return false;

				auto it = clientMap.find(id);
				if (it == clientMap.end()) return false;
				Client& o = *(it->second.get());
				std::shared_lock<std::shared_timed_mutex> clg{ o.lock };
				return (std::abs(c.x - o.x) <= PLAYER_VIEW_SIZE / 2) && (std::abs(c.y - o.y) <= PLAYER_VIEW_SIZE / 2);
			});
		}
	}
	return nearList;
}

void UpdateViewList(Client & c)
{
	auto nearList = GetNearList(c);
	for (auto& playerId : nearList) {
		std::shared_lock<std::shared_timed_mutex> clg{ clientMapLock };
		auto it = clientMap.find(playerId);
		if (it == clientMap.end()) continue;
		auto& player = *it->second.get();
		std::lock(c.lock, player.lock);
		std::unique_lock<std::shared_timed_mutex> myLG{ c.lock, std::adopt_lock };
		std::unique_lock<std::shared_timed_mutex> playerLG{ player.lock, std::adopt_lock };
		auto result = c.viewList.insert(playerId);
		myLG.unlock();
		const bool isInserted = result.second;

		int retval;
		if (isInserted) {
			std::shared_ptr<MsgBase> putPlayerMsg{ new MsgPutObject{ player.id, player.x, player.y, player.color } };
			auto eov = new ExtOverlapped{ c.s, std::move(putPlayerMsg) };
			if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

			result = player.viewList.insert(c.id);
			const bool amIInserted = result.second;
			if (!amIInserted) {
				std::shared_ptr<MsgBase> moveMeMsg{ new MsgMoveObject{c.id, c.x, c.y} };
				eov = new ExtOverlapped{ player.s, std::move(moveMeMsg) };
				if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
			else {
				std::shared_ptr<MsgBase> putMeMsg{ new MsgPutObject{c.id, c.x, c.y, c.color} };
				eov = new ExtOverlapped{ player.s, std::move(putMeMsg) };
				if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
		}
		else {
			result = player.viewList.insert(c.id);
			const bool amIInserted = result.second;
			if (!amIInserted) {
				std::shared_ptr<MsgBase> moveMeMsg{ new MsgMoveObject{c.id, c.x, c.y} };
				auto eov = new ExtOverlapped{ player.s, std::move(moveMeMsg) };
				if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
			else {
				std::shared_ptr<MsgBase> putMeMsg{ new MsgPutObject{c.id, c.x, c.y, c.color} };
				auto eov = new ExtOverlapped{ player.s, std::move(putMeMsg) };
				if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
		}
	}

	std::vector<unsigned int> removedList;
	{
		std::unique_lock<std::shared_timed_mutex> lg{ c.lock };
		std::copy_if(c.viewList.begin(), c.viewList.end(), std::back_inserter(removedList), [&](auto id) {
			return nearList.find(id) == nearList.end();
		});
		for (auto id : removedList) c.viewList.erase(id);
	}

	std::shared_ptr<MsgBase> removeMeMsg{ new MsgRemoveObject{c.id} };
	std::shared_lock<std::shared_timed_mutex> clg{ clientMapLock };
	for (auto& id : removedList) {
		int retval;
		std::shared_ptr<MsgBase> removeMsg{ new MsgRemoveObject{id} };
		auto eov = new ExtOverlapped{ c.s, std::move(removeMsg) };
		if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

		auto it = clientMap.find(id);
		if (it == clientMap.end()) continue;
		auto& player = *it->second.get();
		std::unique_lock<std::shared_timed_mutex> plg{ player.lock };
		retval = player.viewList.erase(c.id);
		if (1 == retval) {
			eov = new ExtOverlapped{ player.s, removeMeMsg };
			if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
		}
	}
}

void ServerMsgHandler::operator()(SOCKET s, const MsgBase & msg)
{
	if (nullptr == client) return;
	switch (msg.type) {
	case MsgType::INPUT_MOVE:
		{
			auto& rMsg = *(const MsgInputMove*)(&msg);
			auto newClientX = max(0, min(client->x + rMsg.dx, BOARD_W - 1));
			auto newClientY = max(0, min(client->y + rMsg.dy, BOARD_H - 1));
			auto prevSectorIdx = PositionToSectorIndex(client->x, client->y);
			auto newSectorIdx = PositionToSectorIndex(newClientX, newClientY);
			{
				std::unique_lock<std::shared_timed_mutex> lg{ client->lock };
				client->x = newClientX;
				client->y = newClientY;
			}
			{
				std::unique_lock<std::shared_timed_mutex> lg{ sectorLock };
				sectorList[prevSectorIdx].erase(client->id);
				sectorList[newSectorIdx].emplace(client->id);
			}

			int retval;
			std::shared_ptr<MsgBase> moveMsg{ new MsgMoveObject{ client->id, client->x, client->y } };
			auto eov = new ExtOverlapped{ client->s, std::move(moveMsg) };
			if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

			UpdateViewList(*client);
		}
		break;
	}
}
