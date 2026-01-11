//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HL2MP SMG1 with grenade selector
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "grenade_ar2.h"
	#include "hl2mp_player.h"
	#include "basegrenade_shared.h"
	#include "prop_combine_ball.h"
	#include "grenade_frag.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponSMG1 C_WeaponSMG1
#endif

#include "tier0/memdbgon.h"

#define SMG1_GRENADE_DAMAGE 100.0f
#define SMG1_GRENADE_RADIUS 250.0f

#define SMG1_GRENADE_CYCLE_DELAY 0.6f

ConVar smg1_ball_radius   ( "gabe+_smg1_ball_radius",   "200", FCVAR_CHEAT );
ConVar smg1_ball_mass     ( "gabe+_smg1_ball_mass",     "150", FCVAR_CHEAT );
ConVar smg1_ball_duration ( "gabe+_smg1_ball_duration", "3.0", FCVAR_CHEAT );
ConVar smg1_frag_damage   ( "gabe+_smg1_frag_damage",   "125", FCVAR_CHEAT );
ConVar smg1_frag_radius   ( "gabe+_smg1_frag_radius",   "275", FCVAR_CHEAT );
ConVar smg1_frag_timer    ( "gabe+_smg1_frag_timer",    "2.5", FCVAR_CHEAT );
ConVar smg1_grenade_delay ( "gabe+_smg1_grenade_delay", "1.0", FCVAR_CHEAT );

class CWeaponSMG1 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponSMG1, CHL2MPMachineGun );

	CWeaponSMG1();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	Precache( void );
	void	AddViewKick( void );
	void	SecondaryAttack( void );
	bool	Reload( void );

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	float	GetFireRate( void ) { return 0.075f; }
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	DECLARE_ACTTABLE();

protected:
	int		m_iGrenadeMode;
	float	m_flNextGrenadeModeSwitch;

private:
	CWeaponSMG1( const CWeaponSMG1 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSMG1, DT_WeaponSMG1 )
BEGIN_NETWORK_TABLE( CWeaponSMG1, DT_WeaponSMG1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSMG1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_smg1, CWeaponSMG1 );
PRECACHE_WEAPON_REGISTER( weapon_smg1 );

acttable_t CWeaponSMG1::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_SMG1, false },
	{ ACT_MP_RUN, ACT_HL2MP_RUN_SMG1, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1, false },
	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_SMG1, false },
};

IMPLEMENT_ACTTABLE( CWeaponSMG1 );

//=========================================================
CWeaponSMG1::CWeaponSMG1()
{
	m_fMinRange1 = 0;
	m_fMaxRange1 = 1400;

	m_iGrenadeMode = 1;
	m_flNextGrenadeModeSwitch = 0.0f;
}

//-----------------------------------------------------------------------------
// Precache
//-----------------------------------------------------------------------------
void CWeaponSMG1::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "grenade_ar2" );
	UTIL_PrecacheOther( "prop_combine_ball" );
	UTIL_PrecacheOther( "npc_grenade_frag" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Reload (SHIFT + Reload cycles grenade type)
//-----------------------------------------------------------------------------
bool CWeaponSMG1::Reload( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return false;

	// SHIFT + Reload = cycle grenade type
	if ( ( pPlayer->m_nButtons & IN_RELOAD ) &&
		 ( pPlayer->m_nButtons & IN_SPEED ) )
	{
		if ( gpGlobals->curtime >= m_flNextGrenadeModeSwitch )
		{
			m_iGrenadeMode++;
			if ( m_iGrenadeMode > 3 )
				m_iGrenadeMode = 1;

#ifndef CLIENT_DLL
			const char *pszName = "AR2";
			if ( m_iGrenadeMode == 2 ) pszName = "ENERGY BALL";
			else if ( m_iGrenadeMode == 3 ) pszName = "FRAG";

			ClientPrint( pPlayer, HUD_PRINTCENTER,
				UTIL_VarArgs( "SMG1 GRENADE: %s", pszName ) );
#endif

			m_flNextGrenadeModeSwitch = gpGlobals->curtime + SMG1_GRENADE_CYCLE_DELAY;
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
		}
		return false;
	}

	// Normal reload
	bool fRet;
	float flCache = m_flNextSecondaryAttack;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = flCache;
		WeaponSound( RELOAD );
		ToHL2MPPlayer( pPlayer )->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: view kick
//-----------------------------------------------------------------------------
void CWeaponSMG1::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

#define EASY_DAMPEN			0.5f
#define MAX_VERTICAL_KICK	1.0f
#define SLIDE_LIMIT			2.0f

	DoMachineGunKick( pPlayer,
		EASY_DAMPEN,
		MAX_VERTICAL_KICK,
		m_fFireDuration,
		SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
// SecondaryAttack (throw selected grenade)
//-----------------------------------------------------------------------------
void CWeaponSMG1::SecondaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecThrow;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );

#ifndef CLIENT_DLL
	if ( m_iGrenadeMode == 1 )
	{
		CGrenadeAR2 *pGrenade =
			(CGrenadeAR2*)Create( "grenade_ar2", vecSrc, vec3_angle, pPlayer );

		if ( pGrenade )
		{
			pGrenade->SetAbsVelocity( vecThrow );
			pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
			pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
			pGrenade->SetThrower( pPlayer );
			pGrenade->SetDamage( SMG1_GRENADE_DAMAGE );
			pGrenade->SetDamageRadius( SMG1_GRENADE_RADIUS );
		}
	}
	else if ( m_iGrenadeMode == 2 )
	{
		Vector vecVel = vecThrow;
		VectorNormalize( vecVel );
		VectorScale( vecVel, 1100.0f, vecVel );

		// Fire the combine ball (engine-native)
		CreateCombineBall(
			vecSrc,
			vecVel,
			smg1_ball_radius.GetFloat(),
			smg1_ball_mass.GetFloat(),
			smg1_ball_duration.GetFloat(),
			pPlayer
		);
	}
	else
	{
		Vector vecVel = vecThrow;
		VectorNormalize( vecVel );
		VectorScale( vecVel, 1000.0f, vecVel );

		AngularImpulse angVel( RandomFloat( -400, 400 ),
							   RandomFloat( -400, 400 ),
							   RandomFloat( -400, 400 ) );

		// Create frag grenade the Valve way
		CBaseGrenade *pFrag = Fraggrenade_Create(
			vecSrc,                     // position
			vec3_angle,                 // angles
			vecVel,                     // velocity
			angVel,                     // angular velocity
			pPlayer,                    // owner
			smg1_frag_timer.GetFloat(), // fuse time
			false                        // combineSpawned (false = normal frag)
		);

		if ( pFrag )
		{
			pFrag->SetDamage( smg1_frag_damage.GetFloat() );
			pFrag->SetDamageRadius( smg1_frag_radius.GetFloat() );
		}
	}
#endif

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer( pPlayer )->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	m_flNextSecondaryAttack = gpGlobals->curtime + smg1_grenade_delay.GetFloat();
}

//-----------------------------------------------------------------------------
Activity CWeaponSMG1::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 ) return ACT_VM_PRIMARYATTACK;
	if ( m_nShotsFired < 3 ) return ACT_VM_RECOIL1;
	if ( m_nShotsFired < 4 ) return ACT_VM_RECOIL2;
	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponSMG1::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t table[] =
	{
		{ 7.0, 0.75 },
		{ 5.0, 0.75 },
		{ 10.0/3.0, 0.75 },
		{ 5.0/3.0, 0.75 },
		{ 1.0, 1.0 },
	};
	return table;
}
