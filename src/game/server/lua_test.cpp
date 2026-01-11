#include "cbase.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

static void LuaSmokeTest()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    lua_close(L);
}
