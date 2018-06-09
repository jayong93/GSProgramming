#include "stdafx.h"
#include "AIQueue.h"
#include "NetworkManager.h"
#include "Globals.h"

bool NpcMsgComp(const NPCMsg & a, const NPCMsg & b)
{
	return a.time > b.time;
}

void NPCMsgCallback(DWORD error, ExtOverlappedNPC *& ov)
{
	using namespace std::chrono;
	auto recvTime = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
	switch (ov->msg.type) {
	case NpcMsgType::MOVE_RANDOM:
	{
		auto locked = objManager.GetSharedCollection();
		const auto it = locked->find(ov->msg.id);
		if (it == locked->end()) break;
		auto& npc = it->second;

		const auto direction = rand() % 4;
		auto newX = npc->x;
		auto newY = npc->y;
		{
			std::unique_lock<std::shared_timed_mutex> lg{ npc->lock };
			switch (direction) {
			case 0: // 왼쪽
				newX -= 1;
				break;
			case 1: // 오른쪽
				newX += 1;
				break;
			case 2: // 위
				newY -= 1;
				break;
			case 3: // 아래
				newY += 1;
				break;
			}
			npc->x = max(0, min(newX, BOARD_W - 1));
			npc->y = max(0, min(newY, BOARD_H - 1));
		}
		locked.unlock();

		npc->UpdateViewList();

		std::shared_lock<std::shared_timed_mutex> lg{ npc->lock };
		if (npc->viewList.size() > 0) {
			npcMsgQueue.Push(NPCMsg(npc->id, NpcMsgType::MOVE_RANDOM, recvTime + 1000));
		}
	}
	break;
	}
}
