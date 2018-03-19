#pragma once
#include <WinSock2.h>

enum class ClientMsg { KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN };
enum class ServerMsg { MOVE_CHARA };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 8, BOARD_H = 8;

void err_quit(LPCTSTR msg);

int RecvAll(SOCKET sock, char * buf, size_t len, int flag);
