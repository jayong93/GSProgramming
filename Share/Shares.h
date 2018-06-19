#pragma once
#include "MsgReconstructor.h"
#include <cassert>

enum class MsgTypeCS { NONE, LOGIN, LOGOUT, MOVE, ATTACK, CHAT, TELEPORT };
enum class MsgTypeSC { NONE, LOGIN_OK, LOGIN_FAIL, POSITION, CHAT, STAT_CHANGE, REMOVE_OBJ, ADD_OBJ, SET_COLOR, OTHER_CHAT, OTHER_STAT_CHANGE };
enum class ObjectType { OBJECT, PLAYER, MELEE, RANGE };

constexpr u_short GS_PORT = 9011;
constexpr int BOARD_W = 400, BOARD_H = 400; // unsigned로 선언하지 말 것. 플레이어 좌표를 min 하는 과정에서 unsigned로 변환되어 -1이 99가 됨
constexpr int VIEW_SIZE = 21;
constexpr int PLAYER_VIEW_SIZE = 21;
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

// C->S

struct MsgLogin : public MsgBase {
	wchar_t gameID[MAX_GAME_ID_LEN];
	MsgLogin(const wchar_t* id) : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::LOGIN } {
		lstrcpynW(gameID, id, MAX_GAME_ID_LEN);
	}
};

struct MsgLogout : public MsgBase {
	MsgLogout() : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::LOGOUT } {}
};

struct MsgInputMove : public MsgBase {
	BYTE direction;
	MsgInputMove(BYTE direction) : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::MOVE }, direction{ direction } {}
};

struct MsgAttack : public MsgBase {
	MsgAttack() : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::ATTACK } {}
};

struct MsgSendChat : public MsgBase {
	wchar_t chat[MAX_CHAT_LEN];
	MsgSendChat(const wchar_t* msg) : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::CHAT } {
		lstrcpynW(chat, msg, MAX_CHAT_LEN);
	}
};

// S->C

struct MsgLoginOK : public MsgBase {
	WORD id;
	WORD x, y;
	WORD hp;
	BYTE level;
	DWORD exp;

	explicit MsgLoginOK(WORD id, WORD x, WORD y, WORD hp, BYTE level, DWORD exp) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::LOGIN_OK }, id{ id }, x{ x }, y{ y }, hp{ hp }, level{ level }, exp{ exp } {}
};

struct MsgLoginFail : public MsgBase {
	MsgLoginFail() : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::LOGIN_FAIL } {}
};

struct MsgSetPosition : public MsgBase {
	WORD id;
	WORD x, y;
	MsgSetPosition(WORD id, WORD x, WORD y) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::POSITION }, id(id), x(x), y(y) {}
};

struct MsgChat : public MsgBase {
	wchar_t msg[MAX_CHAT_LEN];

	MsgChat(WORD from, const wchar_t* msg) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::CHAT } {
		lstrcpynW(this->msg, msg, MAX_CHAT_LEN);
	}
};

struct MsgStatChange : public MsgBase {
	WORD hp;
	BYTE level;
	DWORD exp;

	MsgStatChange(WORD hp, BYTE level, DWORD exp) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::STAT_CHANGE }, hp{ hp }, level{ level }, exp{ exp } {}
};

struct MsgRemoveObject : public MsgBase {
	WORD id;
	explicit MsgRemoveObject(WORD id) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::REMOVE_OBJ }, id{ id } {}
};

struct MsgAddObject : public MsgBase {
	WORD id;
	BYTE objType;
	WORD x, y;

	MsgAddObject(WORD id, ObjectType type, WORD x, WORD y) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::ADD_OBJ }, id{ id }, objType{ (BYTE)type }, x{ x }, y{ y } {}
};

struct MsgSetColor : public MsgBase {
	WORD id;
	Color color;

	MsgSetColor(WORD id, const Color& color) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::SET_COLOR }, id{ id }, color{ color } {}
};

struct MsgOtherChat : public MsgBase {
	WORD from;
	wchar_t msg[MAX_CHAT_LEN];

	MsgOtherChat(WORD from, const wchar_t* msg) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::OTHER_CHAT }, from(from) {
		lstrcpynW(this->msg, msg, MAX_CHAT_LEN);
	}
};

struct MsgOtherStatChange : public MsgBase {
	WORD id;
	WORD hp;
	WORD maxHP;
	BYTE level;
	DWORD exp;

	MsgOtherStatChange(WORD id, WORD hp, WORD maxHP, BYTE level, DWORD exp) : MsgBase{ sizeof(decltype(*this)), MsgTypeSC::OTHER_STAT_CHANGE }, id{ id }, hp{ hp }, maxHP{ maxHP }, level { level }, exp{ exp } {}
};

struct MsgTeleport : public MsgBase {
	WORD x, y;

	MsgTeleport(WORD x, WORD y) : MsgBase{ sizeof(decltype(*this)), MsgTypeCS::TELEPORT }, x{ x }, y{ y } {}
};

#pragma pack(pop)