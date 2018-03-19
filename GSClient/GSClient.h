#pragma once
#include "../Share/Shares.h"

constexpr int CELL_W = 50, CELL_H = 50;

void SendMovePacket(SOCKET sock, UINT_PTR key);
void RecvThreadFunc(HWND hWnd, SOCKET sock);
int RecvAll(SOCKET sock, char* buf, size_t len, int flag);