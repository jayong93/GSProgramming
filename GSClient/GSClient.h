#pragma once
#include "../Share/Shares.h"

constexpr int CELL_W = 50, CELL_H = 50;
constexpr int UM_NETALLOW{ WM_USER + 1 };

void SendMovePacket(SOCKET sock, HWND hWnd, UINT_PTR key);
