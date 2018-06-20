#pragma once
#include "../Share/Shares.h"
#include "GSClient.h"
#include "typedef.h"

struct Chat {
	TCHAR chatMsg[MAX_CHAT_LEN + 1] = { 0, };
	std::chrono::high_resolution_clock::time_point lastChatTime;
	bool isValid{ false };
};

// 클라이언트에서 Object에 대한 접근은 모두 objectManager에서 이루어지기 때문에 object에는 lock이 필요 없음.
struct Object {
private:
	WORD id = 0;
	WORD x = 0, y = 0;
	COLORREF color = RGB(255, 255, 255);
	Chat chat;
	WORD hp = 0;
	WORD maxHP = 0;
	ObjectType type;
	BYTE level;
	DWORD exp;

public:
	Object() {}
	Object(WORD id, WORD x, WORD y) : id{ id }, x(x), y(y){}

	void SetPos(WORD x, WORD y) { this->x = x; this->y = y; }
	auto GetPos() { return std::make_tuple(x, y); }
	auto GetColor() const { return color; }
	void SetColor(const Color& color) { this->color = RGB(color.r, color.g, color.b); }
	auto GetID() const { return id; }
	auto GetType() const { return type; }
	void SetType(ObjectType type) { this->type = type; }
	auto GetHP() const { return hp; }
	void SetHP(WORD hp) { this->hp = hp; }
	auto GetMaxHP() const { return maxHP; }
	void SetMaxHP(WORD maxHP) { this->maxHP = maxHP; }
	auto GetRelativeRect(const int leftTopX, const int leftTopY) const {
		RECT rect;
		const auto relativeX = x - leftTopX;
		const auto relativeY = y - leftTopY;
		rect.left = relativeX * CELL_W;
		rect.right = (relativeX + 1) * CELL_W;
		rect.top = relativeY * CELL_H;
		rect.bottom = (relativeY + 1) * CELL_H;
		return rect;
	}
	auto GetLevel() { return level; }
	auto SetLevel(BYTE lv) {
		level = lv;
		return level;
	}
	auto LevelUp(BYTE num) {
		level += num;
		return level;
	}
	auto GetExp() { return exp; }
	auto SetExp(DWORD exp) { this->exp = exp; return exp; }
	auto ExpUp(DWORD delta) { this->exp += delta; return this->exp; }

	template<typename Func>
	auto AccessToChat(Func func) {
		return func(chat);
	}
};

struct Client {
private:
	MsgReconstructor<ClientMsgHandler> msgRecon;
	SOCKET s;
	WORD id = 0;
	bool connected{ false };

public:
	Client(SOCKET s) : msgRecon{ 100, ClientMsgHandler{*this} }, s{ s } {}
	auto GetSocket() const { return s; }
	auto& GetMessageConstructor() { return msgRecon; }
	auto GetID() const { return id; }
	void SetID(WORD id) { this->id = id; connected = true; }
	bool IsConnected() const { return connected; }
	void Connect(bool t) { connected = t; }
};

class ObjectManager {
public:
	ObjectManager() {}

	bool Insert(std::unique_ptr<Object>&& ptr);
	bool Insert(Object& o);
	bool Remove(WORD id);

	template <typename Func>
	bool Update(WORD id, Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		auto it = data.find(id);
		if (data.end() == it) return false;
		func(*it->second);
		return true;
	}

	template <typename Func>
	auto Access(Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		return func(data);
	}

	static bool IsPlayer(WORD id) { return id < MAX_PLAYER; }

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};
