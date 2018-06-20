#pragma once

#include "ObjectManager.h"
#include "NetworkManager.h"

struct Global {
	ObjectManager objManager;
	NetworkManager networkManager;
	HANDLE iocpObject;
	HWND mainWin;
	Client* myInstance = nullptr;
	TCHAR gameID[MAX_GAME_ID_LEN];
	TCHAR chatMsg[MAX_CHAT_LEN] = { 0, };
	size_t chatLen{ 0 };
};

#ifndef SET_GLOBALS
extern Global global;
#else
Global global;
#endif
