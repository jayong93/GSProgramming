#pragma once

template<typename Value>
struct ShareLocked {
public:
	ShareLocked(std::shared_timed_mutex& lock, Value& v) : lg{ lock }, v{ v } {}
	ShareLocked(ShareLocked<Value>&& o) : lg{ std::move(o.lg) }, v{ o.v } {}
	Value& v;
private:
	std::shared_lock<std::shared_timed_mutex> lg;
};

template<typename Value>
struct UniqueLocked {
public:
	UniqueLocked(std::shared_timed_mutex& lock, Value& v) : lg{ lock }, v{ v } {}
	UniqueLocked(ShareLocked<Value>&& o) : lg{ std::move(o.lg) }, v{ o.v } {}
	Value& v;
private:
	std::unique_lock<std::shared_timed_mutex> lg;
};

template <typename Key, typename Value>
class ObjectManager {
public:
	ShareLocked<Value> GetCollection() { return ShareLocked<decltype(data)>(lock, data); }
private:
	std::shared_timed_mutex lock;
	std::unordered_map<Key, Value> data;
};