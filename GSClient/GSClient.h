#pragma once
#include "../Share/Shares.h"

constexpr int CELL_W = 50, CELL_H = 50;
constexpr int UM_NETALLOW{ WM_USER + 1 };

bool InitNetwork();
void NetworkHandler();
void SendMovePacket(SOCKET sock, UINT_PTR key);
bool SendAll(SOCKET sock, const char* buf, size_t len);
void ProcessMessage(const MsgBase& msg);
void ErrorQuit();
