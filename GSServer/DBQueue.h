#pragma once

#include "DBQuery.h"

class DBMsgQueue {
	std::mutex lock;
	std::list<std::unique_ptr<DBQueryBase>> queue;

public:
	DBMsgQueue() {}
	
	const auto& Top() { return queue.front(); /* Pop하는 thread는 하나 뿐이기 때문에 Top은 Lock을 걸 필요가 없다.*/ }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; queue.pop_front(); }

	void Push(std::unique_ptr<DBQueryBase>&& msg) { std::unique_lock<std::mutex> lg{ lock }; queue.emplace_back(std::move(msg)); }
	void Push(DBQueryBase* msg) { std::unique_lock<std::mutex> lg{ lock }; queue.emplace_back(msg); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return queue.empty(); }
};