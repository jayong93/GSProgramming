#pragma once
#include "../Share/Shares.h"
#include "typedef.h"
#include "NetworkManager.h"

class Object {
	std::mutex lock;
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	std::unordered_set<unsigned int> viewList;

public:
	Object(unsigned int id, short x, short y, Color color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } { o.id = 0; }

	void Move(short dx, short dy);
	void SetPos(short x, short y);
	auto GetID() const { return id; }
	auto GetPos() { ULock{ lock }; return std::make_tuple( x,y ); }
	auto& GetColor() const { return color; }

	template<typename Func>
	auto AccessToViewList(Func func) {
		ULock{ lock };
		return func(viewList);
	}
};

class Client : public Object {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	std::wstring gameID;

public:
	Client(unsigned int id, SOCKET s, Color c, short x, short y, const wchar_t* gameID) : msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, Object{ id, x, y, c }, gameID{ gameID } {}
	Client(Client&& o) : msgRecon{ std::move(o.msgRecon) }, s{ o.s }, gameID{ std::move(o.gameID) }, Object{ std::move(o) } { o.s = 0; }

	auto& GetMessageConstructor() { return msgRecon; }
	auto GetSocket() const { return s; }
	auto& GetGameID() const { return gameID; }
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