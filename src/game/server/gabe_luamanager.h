#ifndef GABE_LUAMANAGER_H
#define GABE_LUAMANAGER_H

#include "tier1/utlvector.h"

extern "C" {
    struct lua_State;
}

struct LuaCoroutine
{
    lua_State* thread;
    float wakeTime;
};

class CGabeLuaManager
{
public:
    CGabeLuaManager();

    void Init();
    void Shutdown();
    void Think();

    lua_State* GetLua() const { return m_pLua; }

    void AddCoroutine(lua_State* thread, float wakeTime);

	bool m_bServerReady;

	void MarkServerReady();

	void RunFile(const char* path);
    void RunString(const char* code);
    int  GetCoroutineCount() const;

private:
    void RunAutorunOnce();
    void ResumeCoroutines();
    void ReportError(const char* where);

private:
    lua_State* m_pLua;
    CUtlVector<LuaCoroutine> m_Coroutines;
    bool m_bAutorunRan;
};

extern CGabeLuaManager g_GabeLua;

#endif
