#pragma once
#include <WinSock2.h>
#include <memory>

enum class MsgType { NONE, INPUT_MOVE, MOVE_CHARA };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 8, BOARD_H = 8;

void err_quit(LPCTSTR msg);

struct MsgBase;

struct ExtOverlapped {
	WSAOVERLAPPED ov;
	std::unique_ptr<MsgBase> msg;

	ExtOverlapped(std::unique_ptr<MsgBase> msg) : msg{ std::move(msg) } { ZeroMemory(&ov, sizeof(ov)); }

	ExtOverlapped(const ExtOverlapped&) = delete;
	ExtOverlapped& operator=(const ExtOverlapped&) = delete;
};

class MsgReconstructor {

};

#pragma pack(push, 1)

struct MsgBase {
	short len{ 0 };
	MsgType type{ MsgType::NONE };

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
	virtual ~MsgBase() {}
};

struct MsgInputMove : public MsgBase {
	byte dx, dy;

	MsgInputMove(byte dx, byte dy) : MsgBase{sizeof(MsgInputMove), MsgType::INPUT_MOVE}, dx{ dx }, dy{ dy } {}
	virtual ~MsgInputMove() {}
};

#pragma pack(pop)