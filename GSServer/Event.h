#pragma once

class EventBase {
public:
	virtual void operator()() const = 0;
};

template <typename Call>
class Event : public EventBase {
public:
	Event(Call&& call) : call{ std::forward<Call>(call) } {}
	Event(const Event<Call>& o) : call{ o.call } {}
	Event(Event<Call>&& o) : call{ std::move(o.call) } {}
	Event<Call>& operator=(const Event<Call>& o) { this->call = o.call; return *this; }
	Event<Call>& operator=(Event<Call>&& o) { this->call = std::move(o.call); return *this; }

	virtual void operator()() const { call(); }
private:
	Call call;
};

template <typename Call>
std::unique_ptr<EventBase> MakeEvent(Call&& call) {
	return std::unique_ptr<EventBase>{new Event<Call>{std::forward<Call>(call)}};
}