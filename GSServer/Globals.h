#pragma once

#include "../Share/Shares.h"
#include "GSServer.h"
#include "ObjectManager.h"
#include "SectorManager.h"
#include "NetworkManager.h"
#include "TimerQueue.h"
#include "DBQueue.h"

#ifndef SET_GLOBALS
#define GLOBAL extern
GLOBAL unsigned int nextId;
GLOBAL unsigned int npcNextId;
GLOBAL ObjectManager objManager;
GLOBAL SectorManager sectorManager;
GLOBAL NetworkManager networkManager;
GLOBAL TimerQueue timerQueue;
GLOBAL DBMsgQueue dbMsgQueue;
GLOBAL HANDLE iocpObject;
GLOBAL SQLHENV henv;
GLOBAL SQLHDBC hdbc;
GLOBAL SQLHSTMT hstmt;
GLOBAL std::random_device rd;
GLOBAL std::mt19937_64 rndGen;
GLOBAL std::uniform_int_distribution<int> posRange;
GLOBAL std::uniform_int_distribution<int> colorRange;
#endif
