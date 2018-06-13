#pragma once

int C_SendMessage(lua_State* L);
int C_GetMyPos(lua_State* L);
int C_SendRandomMove(lua_State* L);
int C_Move(lua_State* L);
int C_GetRandomNumber(lua_State* L);

void RegisterCFunctions(lua_State* L);