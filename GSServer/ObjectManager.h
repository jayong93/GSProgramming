#pragma once
#include "../Share/Shares.h"
#include "typedef.h"
#include "NetworkManager.h"

class Object {
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	ObjectType type;
	std::unordered_set<unsigned int> viewList;

protected:
	std::mutex lock;

public:
	Object(unsigned int id, short x, short y, const Color& color, ObjectType type) : id{ id }, x{ x }, y{ y }, color{ color }, type{ type } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } { o.id = 0; }

	void Move(short dx, short dy);
	void SetPos(short x, short y);
	auto GetID() const { return id; }
	auto GetPos() { ULock{ lock }; return std::make_tuple(x, y); }
	auto& GetColor() const { return color; }
	auto GetType() const { return type; }
	virtual void SendPutMessage(SOCKET s);

	template<typename Func>
	auto AccessToViewList(Func func) {
		ULock{ lock };
		return func(viewList);
	}
};

class HPObject : public Object {
public:
	HPObject(unsigned int id, short x, short y, const Color& color, ObjectType type, unsigned int hp) : Object{ id,x,y,color, type }, hp( hp ), maxHP( hp ) {}

	auto GetHP() { ULock lg{ lock }; return hp; }
	int SetHP(unsigned int hp) {
		ULock lg{ lock };
		this->hp = hp;
		this->hp = min(this->maxHP, max(0, this->hp));
		return this->hp;
	}
	int AddHP(int diff) {
		ULock lg{ lock };
		this->hp += diff;
		this->hp = min(this->maxHP, max(0, this->hp));
		return this->hp;
	}
	auto GetMaxHP() { ULock lg{ lock }; return maxHP; }
	int SetMaxHP(unsigned int maxHP) { ULock lg{ lock }; this->maxHP = maxHP; return this->maxHP; }
	virtual void SendPutMessage(SOCKET s);

private:
	int hp;
	int maxHP;
};

class Client : public HPObject {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	std::wstring gameID;
	unsigned int level;

public:
	Client(unsigned int id, SOCKET s, const Color& c, short x, short y, int hp, const wchar_t* gameID) : msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, HPObject{ id, x, y, c, ObjectType::PLAYER, hp }, gameID{ gameID } {}
	Client(unsigned int id, SOCKET s, const Color& c, short x, short y, const wchar_t* gameID) : Client{ id, s, c, x, y, 100, gameID } {}

	auto& GetMessageConstructor() { return msgRecon; }
	auto GetSocket() const { return s; }
	auto& GetGameID() const { return gameID; }
	auto GetLevel() { ULock lg{ lock }; return level; }
	auto SetLevel(unsigned int lv) {
		ULock lg{ lock };
		level = lv;
		return level;
	}
	auto LevelUp(unsigned int num) {
		ULock lg{ lock };
		level += num;
		return level;
	}
};

class ObjectManager {
public:
	ObjectManager() {}

	bool Insert(std::unique_ptr<Object>&& ptr);
	bool Insert(Object& o);
	bool Remove(unsigned int id);

	template <typename Func>
	bool Update(unsigned int id, Func func) {
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

	template <typename Func>
	bool AccessWithValue(unsigned int id, Func func) {
		return Update(id, [&data{ this->data }, &func](auto& obj) {func(obj, data); });
	}

	static std::unordered_set<unsigned int> GetNearList(unsigned int id, ObjectMap& map);
	template <typename Pred>
	static std::unordered_set<unsigned int> GetNearList(unsigned int id, ObjectMap& map, Pred pred) {
		std::unordered_set<unsigned int> nearList;
		Object* obj = map.at(id).get();

		const auto[x, y] = obj->GetPos();
		auto nearSectors = sectorManager.GetNearSectors(sectorManager.PositionToSectorIndex(x, y));
		for (auto s : nearSectors) {
			std::copy_if(s.begin(), s.end(), std::inserter(nearList, nearList.end()), [&](unsigned int id) {
				if (id == obj->GetID()) return false;

				auto it = map.find(id);
				if (it == map.end()) return false;
				auto o = it->second.get();
				return pred(*obj, *o);
			});
		}
		return nearList;
	}
	static bool IsPlayer(int id) { return id < MAX_PLAYER; }

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};

void UpdateViewList(unsigned int id, ObjectMap& map);