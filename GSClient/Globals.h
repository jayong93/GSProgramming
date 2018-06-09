#pragma once

#include "ObjectManager.h"
#include "NetworkManager.h"

struct Global {
	ObjectManager objManager;
	NetworkManager networkManager;
	HANDLE iocpObject;
	HWND mainWin;
	bool clientsInitialized = false;
	unsigned int clientId = 0;
	unsigned int connectedClientsNum = 0;
};

#ifndef SET_GLOBALS
extern Global global;
#else
Global global;
#endif
