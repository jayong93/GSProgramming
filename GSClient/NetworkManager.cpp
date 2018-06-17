#include "stdafx.h"
#include "ObjectManager.h"
#include "NetworkManager.h"
#include "GSClient.h"
#include "Globals.h"

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
	if (error > 0) err_quit_wsa(error, TEXT("Send"));
}

void NetworkManager::Recv(ExtOverlapped & eov)
{
	if (eov.client == nullptr) return;
	auto& msgRecon = eov.client->msgRecon;
	WSABUF wb;
	wb.buf = msgRecon.GetBuffer();
	wb.len = msgRecon.GetSize();
	eov.isRecv = true;
	DWORD flags{ 0 };
	const int retval = WSARecv(eov.s, &wb, 1, nullptr, &flags, (LPWSAOVERLAPPED)&eov, nullptr);
	int error;
	if (0 == retval || WSA_IO_PENDING == (error = WSAGetLastError())) return;
	if (error > 0) err_quit_wsa(error, TEXT("Recv"));
}

void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		err_quit_wsa(error, TEXT("OverlappedSend"));
	}
}

void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov)
{
	auto eov_ptr = std::unique_ptr<ExtOverlapped>{ ov };
	auto& eov = *eov_ptr.get();
	if (0 != error || 0 == transferred) {
		err_quit_wsa(error, TEXT("OverlappedRecv"));
		return;
	}

	// 하나의 소켓에 대한 Recv는 동시간에 1개 밖에 존재하지 않기 때문에 client에 lock을 할 필요 없음
	auto& msgRecon = eov.client->msgRecon;
	msgRecon.AddSize(transferred);
	msgRecon.Reconstruct(eov.s);

	global.networkManager.RecvNetworkMessage(*eov.client);
}
