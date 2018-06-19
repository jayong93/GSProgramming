#include "stdafx.h"
#include "ObjectManager.h"

bool ObjectManager::Insert(std::unique_ptr<Object>&& ptr) {
	std::unique_lock<std::mutex> lg{ this->lock };
	auto result = this->data.emplace(ptr->GetID(), std::move(ptr));
	return result.second;
}
bool ObjectManager::Insert(Object& o) {
	std::unique_lock<std::mutex> lg{ this->lock };
	auto result = this->data.emplace(o.GetID(), std::unique_ptr<Object>{&o});
	return result.second;
}
bool ObjectManager::Remove(WORD id) {
	std::unique_lock<std::mutex> lg{ this->lock };
	auto removedCount = this->data.erase(id);
	return removedCount == 1;
}
