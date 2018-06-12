#pragma once
#include "../Share/Shares.h"

struct Client;

void RemoveClient(Client* client);
void AcceptThreadFunc();
void WorkerThreadFunc();
void TimerThreadFunc();
void AddNewClient(SOCKET sock, LPCWSTR name, unsigned int xPos, unsigned int yPos);