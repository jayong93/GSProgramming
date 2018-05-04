#pragma once
#include "MsgReconstructor.h"

enum class MsgType { NONE, CS_INPUT_MOVE, SC_GIVE_ID, SC_MOVE_OBJ, SC_PUT_OBJ, SC_REMOVE_OBJ};

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 400, BOARD_H = 400; // unsigned�� �������� �� ��. �÷��̾� ��ǥ�� min �ϴ� �������� unsigned�� ��ȯ�Ǿ� -1�� 99�� ��
constexpr int VIEW_SIZE = 20;
constexpr int PLAYER_VIEW_SIZE = 15;
constexpr unsigned int MAX_PLAYER = 5000;

void err_quit_wsa(LPCTSTR msg);
void err_quit_wsa(DWORD errCode, LPCTSTR msg);

#pragma pack(push, 1)
struct Color {
	unsigned char r, g, b;

	Color(unsigned char r, unsigned char g, unsigned char b) : r{ r }, g{ g }, b{ b } {}
};

// ���� �Լ��� �������� �� ��. vtable ������ �ڷᱸ�� ũ�Ⱑ �ٲ�
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
#pragma pack(pop)