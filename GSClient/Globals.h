#pragma once

#include "ObjectManager.h"
#include "NetworkManager.h"

struct Global {
	ObjectManager objManager;
	NetworkManager networkManager;
	HANDLE iocpObject;
	HWND mainWin;
	Client* myInstance = nullptr;
	TCHAR chatMsg[MAX_CHAT_LEN] = { 0, };
	size_t chatLen{ 0 };
	Locked<std::deque<std::wstring>> chatLogs;
	static constexpr int MAX_CHAT_LOG_NUM{ 7 };
	Locked<std::optional<std::chrono::high_resolution_clock::time_point>> lastChatWindowClose;
};

#ifndef SET_GLOBALS
extern Global global;
#else
Global global;
#endif
