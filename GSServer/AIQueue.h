#pragma once
#include "LuaFunctionCall.h"

enum class NpcMsgType { NONE, MOVE_RANDOM, CALL_LUA_FUNC };

constexpr unsigned int MAX_NPC = 2000;

struct NPCMsg {
	unsigned int id;
	NpcMsgType type;
	long long time;

	NPCMsg(unsigned int id, NpcMsgType type, long long time) : id{ id }, type{ type }, time{ time } {}
};

struct NPCRandomMove : public NPCMsg {
	NPCRandomMove(unsigned int id, long long time) : NPCMsg{ id, NpcMsgType::MOVE_RANDOM, time } {}
};

struct NPCMsgCallLuaFunc : public NPCMsg {
	std::unique_ptr<LuaFunctionCall> call;

	NPCMsgCallLuaFunc(unsigned int id, long long time, std::unique_ptr<LuaFunctionCall>&& call) : NPCMsg{ id, NpcMsgType::CALL_LUA_FUNC, time }, call{ std::move(call) } {}
};

struct NpcMsgComp {
	bool operator()(const std::unique_ptr<NPCMsg>& a, const std::unique_ptr<NPCMsg>& b) { return a->time > b->time; }
};

template <typename Comp>
class NPCMsgQueue {
	std::mutex lock; // Top�� Pop�� ���� ���� Ÿ�̸� thread �ϳ��ۿ� �����Ƿ� shared_mutex ���� �ǹ̰� ����.
	std::priority_queue<std::unique_ptr<NPCMsg>, std::vector<std::unique_ptr<NPCMsg>>, Comp> msgQueue;

public:
	NPCMsgQueue(Comp comp) : msgQueue{ comp } {}
	const NPCMsg& Top() { std::unique_lock<std::mutex> lg{ lock }; return *msgQueue.top().get(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }

	void Push(std::unique_ptr<NPCMsg>&& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(std::move(msg)); }
	void Push(NPCMsg& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(std::unique_ptr<NPCMsg>{&msg}); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.empty(); }
};

struct ExtOverlappedNPC;
void NPCMsgCallback(DWORD error, ExtOverlappedNPC*& ov);