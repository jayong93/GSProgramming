﻿#pragma comment(lib, "ws2_32")

#include "stdafx.h"
#include "GSServer.h"
#include "Globals.h"

std::random_device rd;
std::mt19937_64 rndGen{ rd() };
std::uniform_int_distribution<int> posRange{ 0, min(BOARD_W, BOARD_H) - 1 };
std::uniform_int_distribution<int> colorRange{ 0, 255 };

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

int main() {
	setlocale(LC_ALL, "korean");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	InitDB();

	iocpObject = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	for (auto i = 0; i < MAX_NPC; ++i) {
		auto id = npcNextId++;
		auto npc = std::unique_ptr<Object>{ new AI_NPC(id, posRange(rndGen), posRange(rndGen), Color(colorRange(rndGen), colorRange(rndGen), colorRange(rndGen)), "script/hello.lua") };
		auto[x, y] = npc->GetPos();
		sectorManager.AddToSector(npc->GetID(), x, y);
		objManager.Insert(std::move(npc));
	}

	std::vector<std::thread> threadList;
	threadList.emplace_back(AcceptThreadFunc);
	threadList.emplace_back(TimerThreadFunc);
	threadList.emplace_back(DBThreadFunc);

	auto threadNum = std::thread::hardware_concurrency();
	if (0 == threadNum) threadNum = 1;
	for (auto i = 0; i < threadNum; ++i) { threadList.emplace_back(WorkerThreadFunc); }

	printf_s("Ready to Run\n");
	for (auto& t : threadList) { t.join(); }

	WSACleanup();
}

void RemoveClient(Client* client)
{
	if (nullptr == client) return;
	closesocket(client->GetSocket());
	std::unique_ptr<Object> localClient;
	objManager.Access([client, &localClient](auto& map) {
		auto it = map.find(client->GetID());
		if (map.end() == it) return;
		localClient.swap(it->second);
		map.erase(it);
	});
	if (!localClient) return;
	const auto[x, y] = client->GetPos();
	sectorManager.RemoveFromSector(client->GetID(), x, y);

	objManager.Access([client](auto& map) {
		client->AccessToViewList([client, &map](auto& viewList) {
			for (auto& id : viewList) {
				auto it = map.find(id);
				if (it == map.end()) continue;
				const auto removedCount = it->second->AccessToViewList([id{ client->GetID() }](auto& viewList){
					return viewList.erase(id);
				});
				if (removedCount == 1 && objManager.IsPlayer(id)) {
					auto& player = *reinterpret_cast<Client*>(it->second.get());
					networkManager.SendNetworkMessage(player.GetSocket(), *new MsgRemoveObject{ client->GetID() });
				}
			}
		});
	});
	dbMsgQueue.Push(new DBSetUserData{ hstmt, client->GetGameID(), x, y });
	printf_s("client #%d has disconnected\n", localClient->GetID());
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

	int retval;
	while (true) {
		auto clientSock = WSAAccept(sock, &clientAddr, &addrLen, nullptr, 0);
		if (clientSock == INVALID_SOCKET) err_quit_wsa(TEXT("WSAAccept"));

		TCHAR name[MAX_GAME_ID_LEN + 1] = { 0, };
		int retval{ 0 };
		do {
			retval += recv(clientSock, (char*)name, sizeof(name), 0);
		} while (retval < sizeof(name));

		if (retval == 0 || lstrlen(name) == 0) continue;
		auto result = [clientSock](SQLWCHAR name[], SQLSMALLINT xPos, SQLSMALLINT yPos) {
			AddNewClient(clientSock, name, xPos, yPos);
		};

		dbMsgQueue.Push(MakeGetUserDataQuery(hstmt, name, result));
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
}

void WorkerThreadFunc()
{
	DWORD bytes;
	ULONG_PTR key;
	ExtOverlappedBase* ov;
	while (true) {
		DWORD error{ 0 };
		auto isSuccess = GetQueuedCompletionStatus(iocpObject, &bytes, &key, (LPOVERLAPPED*)&ov, INFINITE);
		if (!ov->isNetworkEvent) {
			auto eov = std::unique_ptr<ExtOverlappedEvent>{ reinterpret_cast<ExtOverlappedEvent*>(ov) };
			(*eov->msg)();
		}
		else {
			auto eov = std::unique_ptr<ExtOverlappedNetwork>{ reinterpret_cast<ExtOverlappedNetwork*>(ov) };
			if (FALSE == isSuccess) error = GetLastError();
			if (eov->isRecv) { RecvCompletionCallback(error, bytes, eov); }
			else { SendCompletionCallback(error, bytes, eov); }
		}
	}
}

void TimerThreadFunc()
{
	using namespace std::chrono;
	while (true) {
		auto startTime = time_point_cast<milliseconds>(high_resolution_clock::now());
		while (!timerQueue.isEmpty()) {
			auto msg = timerQueue.Top();
			if (msg->GetTime() > startTime.time_since_epoch().count()) break;

			auto eov = new ExtOverlappedEvent{ *msg };
			timerQueue.Pop(); // 메세지가 nmsg 안에 이동되었으므로 Pop해도 안전
			PostQueuedCompletionStatus(iocpObject, sizeof(*eov), 0, (LPWSAOVERLAPPED)eov);
		}
		Sleep(0);
	}
}

void DBThreadFunc()
{
	while (true) {
		while (!dbMsgQueue.isEmpty()) {
			auto& msg = dbMsgQueue.Top();
			msg->execute();
			dbMsgQueue.Pop();
		}
		Sleep(0);
	}
}

void InitDB()
{
	SQLRETURN retcode;
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2018_GAME_SERVER", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					if (SQL_SUCCESS != retcode && SQL_SUCCESS_WITH_INFO != retcode) {
						HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
					}
					return;
				}
				HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
			}
			HandleDiagnosticRecord(henv, SQL_HANDLE_ENV, retcode);
		}
		HandleDiagnosticRecord(henv, SQL_HANDLE_ENV, retcode);
	}
}

void AddNewClient(SOCKET sock, LPCWSTR name, unsigned int xPos, unsigned int yPos)
{
	int retval{ 0 };
	unsigned int clientId{ nextId++ };

	if (clientId >= MAX_PLAYER) {
		printf_s("too many clients! no more clients can join to the server\n");
		return;
	}

	auto newClientPtr = std::unique_ptr<Object>(new Client(clientId, sock, Color(colorRange(rndGen), colorRange(rndGen), colorRange(rndGen)), xPos, yPos, name));
	Client& newClient = *reinterpret_cast<Client*>(newClientPtr.get());

	objManager.Access([sock, clientId, &newClientPtr](auto& map) {
		// 이미 NPC들이 들어가있는 것을 감안해야 함.
		if (map.size() - MAX_NPC > MAX_PLAYER) {
			closesocket(sock);
			return;
		}
		map.emplace(clientId, std::move(newClientPtr));
	});

	auto[x, y] = newClient.GetPos();
	sectorManager.AddToSector(clientId, x, y);
	CreateIoCompletionPort((HANDLE)sock, iocpObject, clientId, 0);
	printf_s("client(id: %d, x: %d, y: %d) has connected\n", clientId, xPos, yPos);

	networkManager.SendNetworkMessage(newClient.GetSocket(), *new MsgGiveID{ clientId });
	networkManager.SendNetworkMessage(newClient.GetSocket(), *new MsgPutObject{ clientId, x, y, newClient.GetColor() });

	objManager.Access([clientId](auto& map) {
		UpdateViewList(clientId, map);
	});

	networkManager.RecvNetworkMessage(newClient);
}

