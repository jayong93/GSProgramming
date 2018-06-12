#pragma once

int C_SendMessage(lua_State* L);
int C_GetMyPos(lua_State* L);

void RegisterCFunctions(lua_State* L);