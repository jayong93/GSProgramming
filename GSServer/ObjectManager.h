#pragma once
#include "../Share/Shares.h"
#include "typedef.h"
#include "NetworkManager.h"

class Object {
	WORD id = 0;
	WORD x = 0, y = 0;
	Color color;
	ObjectType type;
	std::unordered_set<unsigned int> viewList;
	bool isDisabled{ false };

protected:
	std::mutex lock;

public:
	Object(WORD id, WORD x, WORD y, const Color& color, ObjectType type) : id{ id }, x{ x }, y{ y }, color{ color }, type{ type } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } { o.id = 0; }

	void Move(short dx, short dy);
	void SetPos(short x, short y);
	auto GetID() const { return id; }
	auto GetPos() { ULock lg{ lock }; return std::make_tuple(x, y); }
	auto& GetColor() const { return color; }
	auto GetType() const { return type; }
	auto IsDisabled() { ULock lg{ lock }; return isDisabled; }
	void SetDisable(bool t) { ULock lg{ lock }; isDisabled = t; }
	virtual void SendPutMessage(SOCKET s);

	template<typename Func>
	auto AccessToViewList(Func func) {
		ULock{ lock };
		return func(viewList);
	}
};

class HPObject : public Object {
public:
	HPObject(WORD id, WORD x, WORD y, const Color& color, ObjectType type, unsigned int hp) : Object{ id,x,y,color, type }, hp(hp), maxHP(hp) {}

	auto GetHP() { ULock lg{ lock }; return hp; }
	int SetHP(WORD hp) {
		ULock lg{ lock };
		this->hp = hp;
		this->hp = min(this->maxHP, max(0, this->hp));
		return this->hp;
	}
	WORD AddHP(short diff) {
		ULock lg{ lock };
		auto chp = (int)this->hp + diff;
		this->hp = min(this->maxHP, max(0, chp));
		return this->hp;
	}
	auto GetMaxHP() { ULock lg{ lock }; return maxHP; }
	int SetMaxHP(WORD maxHP) { ULock lg{ lock }; this->maxHP = maxHP; return this->maxHP; }
	virtual void SendPutMessage(SOCKET s);

private:
	WORD hp;
	WORD maxHP;
};

class Client : public HPObject {
	std::unique_ptr<MessageReceiver> receiver;
	std::wstring gameID;
	BYTE level;
	DWORD exp;
	DWORD nextExp;

public:
	Client(WORD id, MessageReceiver* r, const Color& c, WORD x, WORD y, int hp, const wchar_t* gameID) : receiver{ r }, HPObject{ id, x, y, c, ObjectType::PLAYER, hp }, gameID{ gameID }, level{ 1 }, exp{ 0 }, nextExp{ 100 } { r->owner = this; }
	Client(WORD id, MessageReceiver* r, const Color& c, WORD x, WORD y, const wchar_t* gameID) : Client{ id, r, c, x, y, 100, gameID } {}
	Client(WORD id, MessageReceiver* r, const Color& c, const DBData& data) : Client{ id, r, c, 0, 0, L"" } { SetDBData(data); }

	auto& GetReceiver() const { return *receiver; }
	auto GetSocket() const { return receiver->s; }
	auto& GetGameID() const { return gameID; }
	auto GetLevel() { ULock lg{ lock }; return level; }
	auto SetLevel(BYTE lv) {
		ULock lg{ lock };
		level = lv;
		return level;
	}
	static auto CalcNextExp(BYTE level) {
		return level * 100;
	}
	auto LevelUp(BYTE num) {
		SetHP(GetMaxHP());
		ULock lg{ lock };
		level += num;
		nextExp = CalcNextExp(level);

		return this->exp >= nextExp;
	}
	auto GetExp() { ULock lg{ lock }; return exp; }
	auto SetExp(DWORD exp) { ULock lg{ lock }; this->exp = exp; return exp; }
	auto ExpUp(DWORD delta) { ULock lg{ lock }; this->exp += delta; return this->exp >= nextExp; }
	DBData GetDBData() {
		const auto[x, y] = GetPos();
		const auto hp = GetHP();
		ULock lg{ lock };
		return { gameID, x, y, level, hp, this->exp };
	}
	void SetDBData(const DBData& data) {
		const auto&[gID, x, y, lv, hp, exp] = data;
		SetPos(x, y);
		SetHP(hp);
		gameID = gID; level = lv; this->exp = exp;
	}

	virtual void SendPutMessage(SOCKET s);
};

class ObjectManager {
public:
	ObjectManager() {}

	bool Insert(std::unique_ptr<Object>&& ptr);
	bool Insert(Object& o);
	bool Remove(WORD id);

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

	static std::unordered_set<WORD> GetNearList(WORD id, ObjectMap& map);
	template <typename Pred>
	static std::unordered_set<WORD> GetNearList(WORD id, ObjectMap& map, Pred pred) {
		std::unordered_set<WORD> nearList;
		Object* obj = map.at(id).get();

		const auto[x, y] = obj->GetPos();
		auto nearSectors = sectorManager.GetNearSectors(sectorManager.PositionToSectorIndex(x, y));
		for (auto s : nearSectors) {
			std::copy_if(s.begin(), s.end(), std::inserter(nearList, nearList.end()), [&](WORD id) {
				if (id == obj->GetID()) return false;

				auto it = map.find(id);
				if (it == map.end()) return false;
				auto o = it->second.get();
				if (o->IsDisabled()) return false;
				return pred(*obj, *o);
			});
		}
		return nearList;
	}
	static bool IsPlayer(WORD id) { return id < MAX_PLAYER; }

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};

void UpdateViewList(WORD id, ObjectMap& map);