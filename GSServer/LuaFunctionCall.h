#pragma once

void display_error(lua_State* L);

using LuaArg = std::variant<double, long long, std::string, bool>;

class LuaCall {
public:
	LuaCall(const char* funcName, std::initializer_list<LuaArg> li, unsigned int returnNum);
	LuaCall(const char* funcName, std::vector<LuaArg>&& args, unsigned int returnNum);
	LuaCall(const char* funcName, const std::vector<LuaArg>& args, unsigned int returnNum);
	std::vector<LuaArg> operator()(lua_State* L) const;

private:
	std::string funcName;
	std::vector<LuaArg> args;
	unsigned int returnNum;
};
