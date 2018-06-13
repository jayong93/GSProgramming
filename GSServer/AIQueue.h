#pragma once

class LuaFunctionCall;

enum class NpcMsgType { NONE, MOVE_RANDOM, CALL_LUA_FUNC };

constexpr unsigned int MAX_NPC = 2000;

struct NPCMsg {
	unsigned int id;
	NpcMsgType type;
	long long time;

	NPCMsg(unsigned int id, NpcMsgType type, long long millis);
};

struct NPCRandomMove : public NPCMsg {
	NPCRandomMove(unsigned int id, long long millis) : NPCMsg{ id, NpcMsgType::MOVE_RANDOM, millis } {}
};

struct NPCMsgCallLuaFunc : public NPCMsg {
	std::unique_ptr<LuaFunctionCall> call;

	NPCMsgCallLuaFunc(unsigned int id, long long millis, std::unique_ptr<LuaFunctionCall>&& call);
	NPCMsgCallLuaFunc(unsigned int id, long long millis, LuaFunctionCall& call);
};

struct NpcMsgComp {
	bool operator()(const NPCMsg* a, const NPCMsg* b) { return a->time > b->time; }
};

template <typename Comp>
class NPCMsgQueue {
	std::mutex lock; // Top과 Pop을 쓰는 곳은 타이머 thread 하나밖에 없으므로 shared_mutex 쓰는 의미가 없음.
	std::priority_queue<NPCMsg*, std::vector<NPCMsg*>, Comp> msgQueue;

public:
	NPCMsgQueue(Comp comp) : msgQueue{ comp } {}
	const NPCMsg* Top() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.top(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }
	void Push(NPCMsg& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(&msg); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.empty(); }
};

struct ExtOverlappedNPC;
void NPCMsgCallback(DWORD error, ExtOverlappedNPC*& ov);