#include "stdafx.h"
#include "LuaFunctionCall.h"

template<class T> struct always_false : std::false_type {};

void display_error(lua_State* L) {
	printf("Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

LuaCall::LuaCall(const char * funcName, std::initializer_list<LuaArg> li, unsigned int returnNum) : funcName{ funcName }, args{ li }, returnNum{ returnNum }
{
}

LuaCall::LuaCall(const char * funcName, std::vector<LuaArg>&& args, unsigned int returnNum) : funcName{ funcName }, args{ std::move(args) }, returnNum{ returnNum }
{
}

LuaCall::LuaCall(const char * funcName, const std::vector<LuaArg>& args, unsigned int returnNum) : funcName{ funcName }, args{ args }, returnNum{ returnNum }
{
}

std::vector<LuaArg> LuaCall::operator()(lua_State * L) const
{
	std::vector<LuaArg> output;
	output.reserve(this->returnNum);

	lua_getglobal(L, funcName.c_str());
	for (auto& arg : args) {
		std::visit([L](auto&& v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, long long>) {
				lua_pushinteger(L, v);
			}
			else if constexpr (std::is_same_v<T, double>) {
				lua_pushnumber(L, v);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				lua_pushstring(L, v.c_str());
			}
			else if constexpr (std::is_same_v<T, bool>) {
				lua_pushboolean(L, v);
			}
			else {
				static_assert(always_false<T>::value, "non-exhaustive visitor");
			}
		}, arg);
	}

	if (0 != lua_pcall(L, args.size(), this->returnNum, 0)) {
		display_error(L);
		return output;
	}
	for (auto i = 1; i <= this->returnNum; ++i) {
		switch (lua_type(L, i)) {
		case LUA_TBOOLEAN:
			output.emplace_back((bool)lua_toboolean(L, i));
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i)) output.emplace_back(lua_tointeger(L, i));
			else output.emplace_back(lua_tonumber(L, i));
			break;
		case LUA_TSTRING:
			output.emplace_back(std::string{ lua_tostring(L, i) });
			break;
		default:
			break;
		}
	}
	lua_pop(L, (int)this->returnNum);
	return output;
}
