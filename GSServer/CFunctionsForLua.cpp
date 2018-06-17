#include "stdafx.h"
#define C_FUNCTIONS_FOR_LUA
#include "CFunctionsForLua.h"
#include "../Share/Shares.h"
#include "LuaFunctionCall.h"
#include "Event.h"
#include "TimerQueue.h"
#include "NPC.h"
#include "Globals.h"

int CFSendMessage::operator()(lua_State * L)
{
	std::wstring msg(MAX_CHAT_LEN, L'\0');
	const char* mbMsg = luaL_checkstring(L, 1);
	auto msgLen = strlen(mbMsg);
	if (msgLen > MAX_CHAT_LEN)
		msgLen = MAX_CHAT_LEN;
	size_t converted;
	mbstowcs_s(&converted, &msg[0], msg.size(), mbMsg, msgLen);

	WORD myId = this->obj.GetID();
	auto nearList = objManager.GetNearList(myId, this->map);
	for (auto& id : nearList) {
		if (objManager.IsPlayer(id))
			networkManager.SendNetworkMessageWithID(id, *new MsgChat{ myId , msg.c_str() }, this->map);
	}
	return 0;
}

int CFGetMyPos::operator()(lua_State * L)
{
	auto[x, y] = this->obj.GetPos();
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int CFLuaCallEvent::operator()(lua_State * L)
{
	/*
	args:
	1. delay - 0이하면 PQCM, 0보다 크면 TimerQueue에 삽입
	2. func
	3. arg1
	4. arg2
	...
	n. return_num
	*/

	TimePoint timePoint;
	int argNum = lua_gettop(L);
	double delay = luaL_checknumber(L, 1);
	if (delay > 0) {
		timePoint = std::chrono::high_resolution_clock::now();
	}
	long long delayMilli = delay * 1000;
	const char* funcName = luaL_checkstring(L, 2);
	unsigned int returnNum = luaL_checkinteger(L, -1);

	std::vector<LuaArg> args;
	for (auto i = 3; i < argNum; ++i) {
		switch (lua_type(L, i)) {
		case LUA_TBOOLEAN:
			args.emplace_back((bool)lua_toboolean(L, i));
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i)) args.emplace_back(lua_tointeger(L, i));
			else args.emplace_back(lua_tonumber(L, i));
			break;
		case LUA_TSTRING:
			args.emplace_back(std::string{ lua_tostring(L, i) });
			break;
		default:
			break;
		}
	}

	LuaCall callObj{ funcName, std::move(args), returnNum };
	const auto id = this->obj.GetID();

	auto eventBody = [call = std::move(callObj), id]() {
		objManager.Access([&call, id](auto& map) {
			auto it = map.find(id);
			if (map.end() == it) return;
			auto& npc = *reinterpret_cast<AI_NPC*>(it->second.get());
			npc.lua.Call(call, npc, map);
		});
	};

	// 바로 실행
	if (delay <= 0) {
		PostEvent(std::move(eventBody));
	}
	// 타이머 적용
	else {
		PostTimerEvent(timePoint, delayMilli, std::move(eventBody));
	}

	return 0;
}

int CFMove::operator()(lua_State * L)
{
	short dx = luaL_checkinteger(L, 1);
	short dy = luaL_checkinteger(L, 2);

	this->obj.Move(dx, dy);
	UpdateViewList(this->obj.GetID(), this->map);
	return 0;
}

int CFRandomNumber::operator()(lua_State * L)
{
	/*
	args:
	1. min
	2. max
	*/
	int min = luaL_checkinteger(L, 1);
	int max = luaL_checkinteger(L, 2);

	std::uniform_int_distribution<int> range{ min, max };
	lua_pushnumber(L, range(this->rndGen));
	return 1;
}

std::random_device CFRandomNumber::rd{};
std::mt19937_64 CFRandomNumber::rndGen{ CFRandomNumber::rd() };

void RegisterCFunctions(lua_State * L)
{
	lua_register(L, "c_send_message", CFunc<CFSendMessage>);
	lua_register(L, "c_get_my_pos", CFunc<CFGetMyPos>);
	lua_register(L, "c_lua_call", CFunc<CFLuaCallEvent>);
	lua_register(L, "c_move", CFunc<CFMove>);
	lua_register(L, "c_get_random_num", CFunc<CFRandomNumber>);
}

