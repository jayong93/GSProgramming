#pragma once
#include "../Share/Shares.h"
#include "typedef.h"

constexpr unsigned int MAX_NPC = 2000;

void RemoveClient(Client* client);
void AcceptThreadFunc();
void WorkerThreadFunc();
void TimerThreadFunc();
void DBThreadFunc();
void InitDB();
std::optional<DBData> AddNewClient(SOCKET sock, LPCWSTR name, unsigned int xPos, unsigned int yPos, MessageReceiver& receiver);