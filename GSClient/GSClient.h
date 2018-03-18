#pragma once
#include "../Share/Shares.h"

constexpr int cellW = 50, cellH = 50;

void SendMovePacket(SOCKET sock, UINT_PTR key);
void RecvThreadFunc(HWND hWnd, SOCKET sock);
int RecvAll(SOCKET sock, char* buf, size_t len, int flag);