#pragma once
#include "MsgReconstructor.h"

enum class MsgType { NONE, CS_INPUT_MOVE, CS_TELEPORT, SC_GIVE_ID, SC_MOVE_OBJ, SC_PUT_OBJ, SC_REMOVE_OBJ, SC_CHAT};
enum class ConnectionType {NORMAL, HOTSPOT};

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 400, BOARD_H = 400; // unsigned로 선언하지 말 것. 플레이어 좌표를 min 하는 과정에서 unsigned로 변환되어 -1이 99가 됨
constexpr int VIEW_SIZE = 21;
constexpr int PLAYER_VIEW_SIZE = 15;
constexpr unsigned int MAX_PLAYER = 10000;
constexpr int MAX_GAME_ID_LEN = 10;
constexpr int MAX_CHAT_LEN = 50;

void err_quit_wsa(LPCTSTR msg);
void err_quit_wsa(DWORD errCode, LPCTSTR msg);
void print_network_error(DWORD errCode);

#pragma pack(push, 1)
struct Color {
	unsigned char r, g, b;

	Color(unsigned char r, unsigned char g, unsigned char b) : r{ r }, g{ g }, b{ b } {}
};

// 가상 함수는 선언하지 말 것. vtable 때문에 자료구조 크기가 바뀜
struct MsgBase {
	short len;
	MsgType type;

	MsgBase(short len, MsgType type) : len{ len }, type{ type } {}
};

struct MsgInputMove : public MsgBase {
	short dx, dy;

	MsgInputMove(short dx, short dy) : MsgBase{ sizeof(MsgInputMove), MsgType::CS_INPUT_MOVE }, dx{ dx }, dy{ dy } {}
};

struct MsgGiveID : public MsgBase {
	unsigned int id;

	explicit MsgGiveID(unsigned int id) : MsgBase{ sizeof(MsgGiveID), MsgType::SC_GIVE_ID }, id{ id } {}
};

struct MsgMoveObject : public MsgBase {
	unsigned int id;
	short x, y;
	MsgMoveObject(unsigned int id, short x, short y) : MsgBase{ sizeof(MsgMoveObject), MsgType::SC_MOVE_OBJ }, id{ id }, x{ x }, y{ y } {}
};

struct MsgRemoveObject : public MsgBase {
	unsigned int id;
	explicit MsgRemoveObject(unsigned int id) : MsgBase{ sizeof(MsgRemoveObject), MsgType::SC_REMOVE_OBJ }, id{ id } {}
};

struct MsgPutObject : public MsgBase {
	unsigned int id;
	short x, y;
	Color color;

	MsgPutObject(unsigned int id, short x, short y, Color color) : MsgBase{ sizeof(MsgPutObject), MsgType::SC_PUT_OBJ }, id{ id }, x{ x }, y{ y }, color{ color } {}
};

struct MsgTeleport : public MsgBase {
	short x, y;

	MsgTeleport(short x, short y) : MsgBase{ sizeof(MsgTeleport), MsgType::CS_TELEPORT }, x{ x }, y{ y } {}
};

struct MsgChat : public MsgBase {
	wchar_t msg[MAX_CHAT_LEN + 1];

	MsgChat(const wchar_t* msg) : MsgBase{ sizeof(MsgChat), MsgType::SC_CHAT } {
		lstrcpyn(this->msg, msg, MAX_CHAT_LEN);
	}
};
#pragma pack(pop)