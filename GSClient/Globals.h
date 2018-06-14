#pragma once

#include "ObjectManager.h"
#include "NetworkManager.h"

struct Global {
	ObjectManager objManager;
	NetworkManager networkManager;
	HANDLE iocpObject;
	HWND mainWin;
	Client* myInstance = nullptr;
};

#ifndef SET_GLOBALS
extern Global global;
#else
Global global;
#endif
