#include "stdafx.h"

#define SET_GLOBALS
#include "Globals.h"

unsigned int nextId{ 0 };
unsigned int npcNextId{ MAX_PLAYER };
ObjectManager objManager;
SectorManager sectorManager;
NetworkManager networkManager;
TimerQueue timerQueue;
DBMsgQueue dbMsgQueue;
HANDLE iocpObject;
SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt{ 0 };
std::random_device rd;
std::mt19937_64 rndGen{ rd() };
std::uniform_int_distribution<int> posRange{ 0, min(BOARD_W, BOARD_H) - 1 };
std::uniform_int_distribution<int> colorRange{ 0, 255 };