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