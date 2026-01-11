// Here come the hacks!

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "hl2_player.h"
#include "props.h"
#include "explode.h"

#define MONITOR_MODEL "models/props_lab/monitor02.mdl"
#define HAX_SOUND "vo/npc/male01/hacks01.wav"

class CWeaponHax : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponHax, CBaseHLCombatWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	void Precache(void);
	void PrimaryAttack(void);
	void SpawnMonitor(const Vector &vecSrc, const Vector &vecDir);
};

LINK_ENTITY_TO_CLASS(weapon_hax, CWeaponHax);
PRECACHE_WEAPON_REGISTER(weapon_hax);

IMPLEMENT_SERVERCLASS_ST(CWeaponHax, DT_WeaponHax)
END_SEND_TABLE()

acttable_t CWeaponHax::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PHYSGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PHYSGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PHYSGUN, false },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
};

IMPLEMENT_ACTTABLE(CWeaponHax);

void CWeaponHax::Precache()
{
	PrecacheModel(MONITOR_MODEL);
	PrecacheScriptSound(HAX_SOUND);
	BaseClass::Precache();
}

void CWeaponHax::PrimaryAttack()
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	Vector vecSrc = pPlayer->EyePosition();
	Vector vecDir;
	pPlayer->EyeVectors(&vecDir);

	EmitSound(HAX_SOUND);

	SpawnMonitor(vecSrc, vecDir);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;
}

void CWeaponHax::SpawnMonitor(const Vector &vecSrc, const Vector &vecDir)
{
	CPhysicsProp *pMonitor = dynamic_cast<CPhysicsProp *>(CreateEntityByName("prop_physics"));
	if (!pMonitor)
		return;

	pMonitor->SetModel(MONITOR_MODEL);
	pMonitor->SetAbsOrigin(vecSrc + vecDir * 32);
	pMonitor->SetAbsAngles(vec3_angle);
	pMonitor->SetOwnerEntity(GetOwner());
	pMonitor->SetSolid(SOLID_VPHYSICS);
	pMonitor->SetMoveType(MOVETYPE_VPHYSICS);
	pMonitor->SetCollisionGroup(COLLISION_GROUP_NONE);

	DispatchSpawn(pMonitor);
	pMonitor->Activate();

	IPhysicsObject *pPhys = pMonitor->VPhysicsGetObject();
	if (pPhys)
	{
		pPhys->EnableGravity(false);
		pPhys->Wake();
		Vector launchVelocity = vecDir * 2500.0f;
		pPhys->SetVelocity(&launchVelocity, NULL);
	}
}
