#pragma once

enum class NpcMsgType { NONE, MOVE_RANDOM };

constexpr unsigned int MAX_NPC = 200000;

struct NPCMsg {
	unsigned int id;
	NpcMsgType type;
	long long time;

	NPCMsg(unsigned int id, NpcMsgType type, long long time) : id{ id }, type{ type }, time{ time } {}
};

struct NpcMsgComp {
	bool operator()(const NPCMsg& a, const NPCMsg&b) { return a.time > b.time; }
};

template <typename Comp>
class NPCMsgQueue {
	std::mutex lock; // Top과 Pop을 쓰는 곳은 타이머 thread 하나밖에 없으므로 shared_mutex 쓰는 의미가 없음.
	std::priority_queue<NPCMsg, std::vector<NPCMsg>, Comp> msgQueue;

public:
	NPCMsgQueue(Comp comp) : msgQueue{ comp } {}
	const NPCMsg& Top() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.top(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }

	void Push(NPCMsg&& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(std::move(msg)); }
	void Push(NPCMsg& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(msg); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.empty(); }
};
