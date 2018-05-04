#pragma once
#include "../Share/Shares.h"

using Sector = std::unordered_set<unsigned int>;
struct Client;

enum class NpcMsgType { NONE, MOVE_RANDOM };

constexpr unsigned int MAX_PLAYER = 5000;

struct ServerMsgHandler {
	Client* client;

	ServerMsgHandler() : client{ nullptr } {}
	ServerMsgHandler(Client& c) : client{ &c } {}
	void operator()(SOCKET s, const MsgBase& msg);
	~ServerMsgHandler() {}
};

struct Object {
	std::shared_timed_mutex lock;
	unsigned int id=0;
	short x=0, y=0;
	Color color;
	std::unordered_set<unsigned int> viewList;

	Object() : color{ 0,0,0 } {}
	Object(unsigned int id, short x, short y, Color& color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } {}
};

using NPC = Object;

struct Client : public Object {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;

	Client(){}
	Client(unsigned int id, SOCKET s, Color c, short x, short y) : msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, Object{ id, x, y, c } {}
	Client(Client&& o) : msgRecon{ std::move(o.msgRecon) }, s{ o.s }, Object{ std::move(o) } {}
	Client& operator=(Client&& o) {
		id = o.id;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
		color = o.color;
		x = o.x;
		y = o.y;
		viewList = std::move(o.viewList);
	}
};

struct NPCMsg {
	unsigned int id;
	NpcMsgType type;
	long long time;
};

template <typename Comp>
class NPCMsgQueue {
	std::mutex lock; // Top과 Pop을 쓰는 곳은 타이머 thread 하나밖에 없으므로 shared_mutex 쓰는 의미가 없음.
	std::priority_queue<NPCMsg, std::vector<NPCMsg>, Comp> msgQueue;

public:
	NPCMsgQueue(Comp comp) : msgQueue{ comp } {}
	const NPCMsg& Top() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.top(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }

	template <typename T>
	void Push(T&& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(std::forward(msg)); }
};

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	SOCKET s;
	Client* client;
	std::shared_ptr<MsgBase> msg; // 동일한 메시지를 Broadcasting하는 경우, shared_ptr로 공유해서 사용
	bool isRecv{ false };

	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>& msg) : s{ s }, client{ nullptr }, msg{ msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, std::shared_ptr<MsgBase>&& msg) : s{ s }, client{ nullptr }, msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlapped(SOCKET s, Client& client) : s{ s }, client{ &client } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

struct ExtOverlappedNPC {
	WSAOVERLAPPED ov;
	NPCMsg msg;

	ExtOverlappedNPC(const NPCMsg& msg) : msg{ msg } { ZeroMemory(&ov, sizeof(ov)); }
	ExtOverlappedNPC(const ExtOverlappedNPC&) = delete;
	ExtOverlappedNPC& operator=(const ExtOverlappedNPC&) = delete;
};

int OverlappedRecv(ExtOverlapped& ov);
int OverlappedSend(ExtOverlapped& ov);
void SendCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void RecvCompletionCallback(DWORD error, DWORD transferred, ExtOverlapped*& ov);
void NPCMsgCallback(DWORD error, ExtOverlappedNPC*& ov);
void RemoveClient(Client& client);
void AcceptThreadFunc();
void WorkerThreadFunc();
void TimerThreadFunc();
unsigned int PositionToSectorIndex(unsigned int x, unsigned int y);
std::vector<Sector*> GetNearSectors(unsigned int sectorIdx);
std::unordered_set<unsigned int> GetNearList(Object& c);
void UpdateViewList(Object& c);