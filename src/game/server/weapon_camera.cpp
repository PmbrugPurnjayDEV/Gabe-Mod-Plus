//========= Copyright Valve Corporation ============//
//
// Camera
//
//==================================================

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
    #include "c_hl2mp_player.h"
    #include "iclientmode.h"
#else
    #include "hl2mp_player.h"
#endif

#include "tier0/memdbgon.h"

// -----------------------------------------------------------------------------
// Shared weapon class
// -----------------------------------------------------------------------------

class CWeaponCamera : public CBaseHL2MPCombatWeapon
{
public:
    DECLARE_CLASS( CWeaponCamera, CBaseHL2MPCombatWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponCamera();

    virtual void PrimaryAttack( void );
    virtual bool Deploy( void );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCamera, DT_WeaponCamera )

BEGIN_NETWORK_TABLE( CWeaponCamera, DT_WeaponCamera )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCamera )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_camera, CWeaponCamera );
PRECACHE_WEAPON_REGISTER( weapon_camera );

// -----------------------------------------------------------------------------
// Weapon logic
// -----------------------------------------------------------------------------

CWeaponCamera::CWeaponCamera()
{
    m_fMinRange1 = 0;
    m_fMaxRange1 = 0;
}

bool CWeaponCamera::Deploy()
{
    return BaseClass::Deploy();
}

void CWeaponCamera::PrimaryAttack()
{
    CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer )
        return;

    WeaponSound( SINGLE );

    // Trigger camera capture
    engine->ClientCommand( pPlayer->edict(), "jpeg\n" );
    ClientPrint( pPlayer, HUD_PRINTTALK, "Photo captured (.jpg)" );

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.8f;
}

// -----------------------------------------------------------------------------
// CLIENT SIDE CAMERA SYSTEM (GMod 6 STYLE)
// -----------------------------------------------------------------------------
#ifdef CLIENT_DLL

static bool  g_bCameraActive = false;
static float g_flCameraRestoreTime = 0.0f;

CON_COMMAND( camera_take, "Internal camera capture (do not call manually)" )
{
    if ( g_bCameraActive )
        return;

    g_bCameraActive = true;
    g_flCameraRestoreTime = gpGlobals->curtime + 0.1f;

    // Clean frame setup
    engine->ExecuteClientCmd( "r_drawviewmodel 0" );
    engine->ExecuteClientCmd( "cl_drawhud 0" );
    engine->ExecuteClientCmd( "fov 25" );

    // Actual capture (engine handles .tga)
    engine->ExecuteClientCmd( "screenshot" );
}

// Called every frame (cheap + safe)
static void CameraClientThink()
{
    if ( !g_bCameraActive )
        return;

    if ( gpGlobals->curtime >= g_flCameraRestoreTime )
    {
        engine->ExecuteClientCmd( "fov 0" );
        engine->ExecuteClientCmd( "cl_drawhud 1" );
        engine->ExecuteClientCmd( "r_drawviewmodel 1" );
        g_bCameraActive = false;
    }
}

// Hook into client frame update
class CCameraThinkHook : public CAutoGameSystem
{
public:
    virtual void Update( float frametime )
    {
        CameraClientThink();
    }
};

static CCameraThinkHook g_CameraThinkHook;

#endif
