#pragma once
#include "Event.h"

using TimePoint = std::chrono::high_resolution_clock::time_point;

class TimerEventBase : public EventBase {
public:
	TimerEventBase(long long delay) {
		using namespace std::chrono;
		this->time = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count() + delay;
	}
	TimerEventBase(TimePoint& timePoint, long long delay) {
		using namespace std::chrono;
		this->time = time_point_cast<milliseconds>(timePoint).time_since_epoch().count() + delay;
	}

	long long GetTime() const { return this->time; }
private:
	long long time;
};

template <typename Call>
class TimerEvent : public TimerEventBase {
public:
	TimerEvent(Call&& call, long long delay) : TimerEventBase{ delay }, call{ std::forward<Call>(call) } {}
	TimerEvent(Call&& call, TimePoint& timePoint, long long delay) : TimerEventBase{ timePoint, delay }, call { std::forward<Call>(call) } {}
	TimerEvent(const TimerEvent<Call>& o) : TimerEventBase{ o }, call { o.call } {}
	TimerEvent(TimerEvent<Call>&& o) : TimerEventBase{ std::move(o) }, call{ std::move(o.call) } {}
	TimerEvent<Call>& operator=(const TimerEvent<Call>& o) {
		TimerEventBase::operator=(o);
		this->call = o.call;
		return *this;
	}
	TimerEvent<Call>& operator=(TimerEvent<Call>&& o) {
		TimerEventBase::operator=(std::move(o));
		this->call = std::move(o.call);
		return *this;
	}

	virtual void operator()() const { call(); }
private:
	Call call;
};

template <typename Call>
std::unique_ptr<TimerEventBase> MakeTimerEvent(Call&& call, long long delay) {
	return std::unique_ptr<TimerEventBase>{new TimerEvent<Call>{ std::forward<Call>(call), delay }};
}

template <typename Call>
std::unique_ptr<TimerEventBase> MakeTimerEvent(Call&& call, TimePoint& timePoint, long long delay) {
	return std::unique_ptr<TimerEventBase>{new TimerEvent<Call>{ std::forward<Call>(call), timePoint, delay }};
}

struct TimerEventComp {
	bool operator()(const TimerEventBase* a, const TimerEventBase* b) { return a->GetTime() > b->GetTime(); }
};

class TimerQueue {
	std::mutex lock;
	std::priority_queue<TimerEventBase*, std::vector<TimerEventBase*>, TimerEventComp> msgQueue;

public:
	TimerQueue() : msgQueue{ TimerEventComp() } {}
	const TimerEventBase* Top() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.top(); }
	void Pop() { std::unique_lock<std::mutex> lg{ lock }; msgQueue.pop(); }
	void Push(TimerEventBase& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(&msg); }
	void Push(TimerEventBase&& msg) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(&msg); }
	void Push(std::unique_ptr<TimerEventBase>&& ptr) { std::unique_lock<std::mutex> lg{ lock }; msgQueue.push(ptr.release()); }
	bool isEmpty() { std::unique_lock<std::mutex> lg{ lock }; return msgQueue.empty(); }
};
