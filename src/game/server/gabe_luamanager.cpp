#include "cbase.h"
#include "gabe_luamanager.h"
#include "filesystem.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

CGabeLuaManager g_GabeLua;

// ------------------------------------------------------------
// Lua bindings
// ------------------------------------------------------------

static int lua_wait(lua_State* L)
{
    float seconds = (float)luaL_checknumber(L, 1);
    lua_pushnumber(L, gpGlobals->curtime + seconds);
    return lua_yield(L, 1);
}

static int lua_msg(lua_State* L)
{
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
    {
        const char* s = lua_tostring(L, i);
        if (s) Msg("%s", s);
        if (i < n) Msg("\t");
    }
    Msg("\n");
    return 0;
}

static int lua_spawn(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_State* mainL = g_GabeLua.GetLua();
    lua_State* co = lua_newthread(mainL);
    lua_pop(mainL, 1);

    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);

    g_GabeLua.AddCoroutine(co, gpGlobals->curtime);
    return 0;
}

// ------------------------------------------------------------
// Manager
// ------------------------------------------------------------

CGabeLuaManager::CGabeLuaManager()
{
    m_pLua = NULL;
    m_bAutorunRan = false;
	m_bServerReady = false;
}

void CGabeLuaManager::MarkServerReady()
{
    Msg("[Lua] Server activated\n");
    m_bServerReady = true;
}

void CGabeLuaManager::Init()
{
    Msg("[Lua] Init\n");

    m_pLua = luaL_newstate();
    if (!m_pLua)
    {
        Warning("[Lua] Failed to create Lua state\n");
        return;
    }

    luaL_openlibs(m_pLua);

    // Register API
    lua_register(m_pLua, "wait", lua_wait);
    lua_register(m_pLua, "spawn", lua_spawn);
    lua_register(m_pLua, "Msg", lua_msg);

    // Override print
    lua_getglobal(m_pLua, "Msg");
    lua_setglobal(m_pLua, "print");

    // Lifecycle flags
    m_bAutorunRan   = false;
    m_bServerReady  = false;

    Msg("[Lua] Init complete\n");
}

void CGabeLuaManager::Shutdown()
{
    Msg("[Lua] Shutdown\n");

    m_Coroutines.RemoveAll();

    if (m_pLua)
    {
        lua_close(m_pLua);
        m_pLua = NULL;
    }

    m_bAutorunRan = false;
}

void CGabeLuaManager::Think()
{
    if (!m_pLua)
        return;

    if (!m_bServerReady)
        return;

    if (!m_bAutorunRan)
        RunAutorunOnce();

    ResumeCoroutines();
}

void CGabeLuaManager::RunAutorunOnce()
{
    m_bAutorunRan = true;

    char fullpath[MAX_PATH];
    g_pFullFileSystem->RelativePathToFullPath(
        "lua/autorun.lua",
        "MOD",
        fullpath,
        sizeof(fullpath)
    );

    Msg("[Lua] Running autorun: %s\n", fullpath);

    if (luaL_dofile(m_pLua, fullpath) != 0)
        ReportError("autorun");
}

void CGabeLuaManager::AddCoroutine(lua_State* thread, float wakeTime)
{
    LuaCoroutine c;
    c.thread = thread;
    c.wakeTime = wakeTime;
    m_Coroutines.AddToTail(c);
}

void CGabeLuaManager::ResumeCoroutines()
{
    for (int i = 0; i < m_Coroutines.Count(); )
    {
        LuaCoroutine& c = m_Coroutines[i];

        if (gpGlobals->curtime < c.wakeTime)
        {
            ++i;
            continue;
        }

        int result = lua_resume(c.thread, 0);

        if (result == LUA_YIELD)
        {
            c.wakeTime = (float)lua_tonumber(c.thread, -1);
            lua_pop(c.thread, 1);
            ++i;
        }
        else
        {
            if (result != 0)
                Warning("[Lua] Coroutine error: %s\n", lua_tostring(c.thread, -1));

            m_Coroutines.Remove(i);
        }
    }
}

void CGabeLuaManager::ReportError(const char* where)
{
    const char* err = lua_tostring(m_pLua, -1);
    Warning("[Lua] Error in %s: %s\n", where, err ? err : "unknown");
    lua_pop(m_pLua, 1);
}

void CGabeLuaManager::RunFile(const char* path)
{
    if (!m_pLua)
        return;

    char fullpath[MAX_PATH];
    g_pFullFileSystem->RelativePathToFullPath(
        path,
        "MOD",
        fullpath,
        sizeof(fullpath)
    );

    Msg("[Lua] Running file: %s\n", fullpath);

    if (luaL_dofile(m_pLua, fullpath) != 0)
        ReportError(path);
}

void CGabeLuaManager::RunString(const char* code)
{
    if (!m_pLua)
        return;

    if (luaL_dostring(m_pLua, code) != 0)
        ReportError("RunString");
}

int CGabeLuaManager::GetCoroutineCount() const
{
    return m_Coroutines.Count();
}

static void CC_LuaRun(const CCommand& args)
{
    if (args.ArgC() < 2)
    {
        Msg("Usage: lua_run \"lua code\"\n");
        return;
    }

    if (!g_GabeLua.GetLua())
    {
        Warning("[Lua] VM not initialized\n");
        return;
    }

    g_GabeLua.RunString(args.ArgS());
}

static void CC_LuaDoFile(const CCommand& args)
{
    if (args.ArgC() < 2)
    {
        Msg("Usage: lua_dofile <file.lua>\n");
        return;
    }

    char path[MAX_PATH];
    Q_snprintf(path, sizeof(path), "lua/%s", args[1]);

    g_GabeLua.RunFile(path);
}

static void CC_LuaReload(const CCommand& args)
{
    Msg("[Lua] Reloading Lua VM...\n");

    g_GabeLua.Shutdown();
    g_GabeLua.Init();
}

static void CC_LuaStatus(const CCommand& args)
{
    lua_State* L = g_GabeLua.GetLua();

    if (L)
        Msg("[Lua] VM active (%p)\n", L);
    else
        Msg("[Lua] VM NOT initialized\n");
}

static void CC_LuaCoroutines(const CCommand& args)
{
    Msg("[Lua] Active coroutines: %d\n", g_GabeLua.GetCoroutineCount());
}

ConCommand lua_run(
    "lua_run",
    CC_LuaRun,
    "Execute Lua code: lua_run \"print('hello')\"",
    FCVAR_NONE
);

ConCommand lua_dofile(
    "lua_dofile",
    CC_LuaDoFile,
    "Execute Lua file from lua/ directory",
    FCVAR_NONE
);

ConCommand lua_reload(
    "lua_reload",
    CC_LuaReload,
    "Reload Lua VM and rerun autorun",
    FCVAR_NONE
);

ConCommand lua_vmstatus(
    "lua_vmstatus",
    CC_LuaStatus,
    "Print Lua VM status",
    FCVAR_NONE
);

ConCommand lua_coroutines(
    "lua_coroutines",
    CC_LuaCoroutines,
    "Print active Lua coroutine count",
    FCVAR_NONE
);

