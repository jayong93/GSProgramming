#pragma comment(lib, "ws2_32")

#include "stdafx.h"
#include "GSServer.h"

int nextId{ 0 };
std::deque<unsigned int> freeIdQueue;
std::map<unsigned int, std::unique_ptr<Client>> clientMap;

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

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
	std::uniform_int_distribution<int> posRange{ 0, BOARD_W-1 };
	std::uniform_int_distribution<int> colorRange{ 0, 255 };

	int retval;
	while (true) {
		auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);
		if (clientSock == INVALID_SOCKET) err_quit_wsa(TEXT("WSAAccept"));
		printf_s("client has connected\n");
		unsigned int clientId;
		if (freeIdQueue.empty()) clientId = nextId++;
		else {
			clientId = freeIdQueue.front(); freeIdQueue.pop_front();
		}
		clientMap.emplace(std::make_pair(clientId, std::make_unique<Client>(clientId, clientSock, Color(colorRange(rndGen), colorRange(rndGen), colorRange(rndGen)), posRange(rndGen), posRange(rndGen))));
		Client& newClient = *clientMap[clientId];

		// 클라이언트 접속 메시지 브로드캐스팅
		std::shared_ptr<MsgBase> inMsg{ new MsgClientIn{ newClient.id, newClient.x, newClient.y, newClient.color } };
		for (auto& c : clientMap) {
			auto eov = new ExtOverlapped{ c.second->s, inMsg };
			if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));

			if (c.first != clientId) {
				std::shared_ptr<MsgBase> otherInMsg{ new MsgClientIn{ c.second->id, c.second->x, c.second->y, c.second->color } };
				eov = new ExtOverlapped{ newClient.s, std::move(otherInMsg) };
				if (0 < (retval = OverlappedSend(*eov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
		}

		auto eov = new ExtOverlapped(clientSock, newClient);
		if (0 < (retval = OverlappedRecv(*eov))) err_quit_wsa(retval, TEXT("OverlappedRecv"));
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
}

int OverlappedRecv(ExtOverlapped & ov)
{
	if (ov.client == nullptr) return -1;
	WSABUF wb;
	wb.buf = ov.client->msgRecon.GetBuffer();
	wb.len = ov.client->msgRecon.GetSize();
	DWORD flags{ 0 };
	const int retval = WSARecv(ov.s, &wb, 1, nullptr, &flags, (LPWSAOVERLAPPED)&ov, RecvCompletionCallback);
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
	const int retval = WSASend(ov.s, &wb, 1, nullptr, 0, (LPWSAOVERLAPPED)&ov, SendCompletionCallback);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return 0;
	return error;
}

void SendCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		RemoveClient(*eov.client);
		return;
	}
	delete &eov;
}

void RecvCompletionCallback(DWORD error, DWORD transferred, LPWSAOVERLAPPED ov, DWORD flag)
{
	auto& eov = *reinterpret_cast<ExtOverlapped*>(ov);
	if (0 != error || 0 == transferred) {
		RemoveClient(*eov.client);
		return;
	}
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
	auto id = client.id;
	freeIdQueue.push_front(id);
	clientMap.erase(id);
	std::shared_ptr<MsgBase> msg{ new MsgClientOut{id} };
	for (auto& c : clientMap) {
		auto ov = new ExtOverlapped{ c.second->s, msg };
		OverlappedSend(*ov);
	}
}

void ServerMsgHandler::ProcessMessage(SOCKET s, const MsgBase & msg)
{
	switch (msg.type) {
	case MsgType::INPUT_MOVE:
		{
			auto& rMsg = *(const MsgInputMove*)(&msg);
			client.x = max(0, min(client.x + rMsg.dx, BOARD_W - 1));
			client.y = max(0, min(client.y + rMsg.dy, BOARD_H - 1));
			std::shared_ptr<MsgBase> moveMsg{ new MsgMoveCharacter{ client.id, client.x, client.y } };
			for (auto& c : clientMap) {
				auto ov = new ExtOverlapped{ c.second->s, moveMsg };
				int retval;
				if (0 < (retval = OverlappedSend(*ov))) err_quit_wsa(retval, TEXT("OverlappedSend"));
			}
		}
		break;
	}
}
