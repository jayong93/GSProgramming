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
	std::mutex lock; // Top�� Pop�� ���� ���� Ÿ�̸� thread �ϳ��ۿ� �����Ƿ� shared_mutex ���� �ǹ̰� ����.
	std::priority_queue<NPCMsg, std::vector<NPCMsg>, Comp> msgQueue;

public:
	NPCMsgQueue(Comp comp) : msgQueue{ comp } {}
	const NPCMsg& Top() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.top(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }

	void Push(NPCMsg&& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(std::move(msg)); }
	void Push(NPCMsg& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(msg); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.empty(); }
};
