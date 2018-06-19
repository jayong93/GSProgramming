#pragma once
#include "MsgReconstructor.h"
#include <cassert>

enum class MsgTypeCS { NONE, CS_MOVE_UP, CS_MOVE_DOWN, CS_MOVE_LEFT, CS_MOVE_RIGHT, CS_CHAT, CS_TELEPORT, CS_ATTACK };
enum class MsgTypeSC { NONE, SC_MOVE_OBJ, SC_PUT_OBJ, SC_REMOVE_OBJ, SC_CHAT, SC_GIVE_ID, SC_DETAIL_DATA, SC_SET_HP, SC_SET_MAX_HP, SC_CHANGE_LEVEL, SC_ATTACKED };
enum class ConnectionType { NORMAL, HOTSPOT };
enum class ObjectType { OBJECT, PLAYER, MELEE, RANGE };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 400, BOARD_H = 400; // unsigned로 선언하지 말 것. 플레이어 좌표를 min 하는 과정에서 unsigned로 변환되어 -1이 99가 됨
constexpr int VIEW_SIZE = 21;
constexpr int PLAYER_VIEW_SIZE = 15;
constexpr unsigned int MAX_PLAYER = 10000;
constexpr int MAX_GAME_ID_LEN = 10;
constexpr int MAX_CHAT_LEN = 100;

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
	unsigned char len;
	unsigned char type;

	template<typename Type>
	MsgBase(unsigned char len, Type type) : len{ len }, type{ (unsigned char)type } {}
};

struct MsgInputMove : public MsgBase {
	MsgInputMove(MsgTypeCS type) : MsgBase{ sizeof(decltype(*this)), type } {}
};

struct MsgGiveID : public MsgBase {
	unsigned int id;

	explicit MsgGiveID(unsigned int id) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_GIVE_ID }, id{ id } {}
};

struct MsgMoveObject : public MsgBase {
	WORD id;
	WORD x, y;
	MsgMoveObject(unsigned int id, short x, short y) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_MOVE_OBJ }, id( id ), x( x ), y( y ) {}
};

struct MsgRemoveObject : public MsgBase {
	WORD id;
	explicit MsgRemoveObject(unsigned int id) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_REMOVE_OBJ }, id( id ) {}
};

struct MsgPutObject : public MsgBase {
	WORD id;
	WORD x, y;

	MsgPutObject(unsigned int id, short x, short y) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_PUT_OBJ }, id( id ), x( x ), y( y ) {}
};

struct MsgDetailData : public MsgBase {
	unsigned int id;
	Color color;
	ObjectType objType;

	MsgDetailData(unsigned int id, Color color, ObjectType type) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_DETAIL_DATA }, id{ id }, color { color }, objType{ type } {}
};

struct MsgTeleport : public MsgBase {
	short x, y;

	MsgTeleport(short x, short y) : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::CS_TELEPORT }, x{ x }, y{ y } {}
};

struct MsgChat : public MsgBase {
	WORD from;
	wchar_t msg[MAX_CHAT_LEN];

	MsgChat(unsigned int from, const wchar_t* msg) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_CHAT }, from( from ) {
		lstrcpyn(this->msg, msg, MAX_CHAT_LEN);
	}
};

struct MsgSetHP : public MsgBase {
	unsigned int id;
	int hp;

	MsgSetHP(unsigned int id, int hp) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_SET_HP }, id{ id }, hp{ hp } {}
};

struct MsgSetMaxHP : public MsgBase {
	unsigned int id;
	int maxHP;

	MsgSetMaxHP(unsigned int id, int maxHP) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_SET_MAX_HP }, id{ id }, maxHP{ maxHP } {}
};

struct MsgChangeLevel : public MsgBase {
	unsigned int id;
	int level;

	MsgChangeLevel(unsigned int id, int level) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_CHANGE_LEVEL }, id{ id }, level{ level } {}
};

struct MsgAttacked : public MsgBase {
	unsigned int id;
	unsigned int from;
	int damage;

	MsgAttacked(unsigned int id, unsigned int from, int damage) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SC_CHANGE_LEVEL }, id{ id }, from{ from }, damage{ damage } {}
};
#pragma pack(pop)