#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "explode.h"
#include "hl2_player.h"
#include "game.h"
#include "entityflame.h"
#include "IEffects.h"
#include "gabeplus_shared.h"
#include "in_buttons.h"
#include "vphysics/constraints.h"
#include "rope.h"
#include "props.h"


extern IEffects* g_pEffects;

class CWeaponMultitool;

const struct ColorPreset {
	int r, g, b;
	const char* name;
} g_ColorPresets[] = {
	{255, 0, 0, "Red"},
	{255, 165, 0, "Orange"},
	{255, 255, 0, "Yellow"},
	{0, 255, 0, "Green"},
	{0, 0, 255, "Blue"},
	{75, 0, 130, "Indigo"},
	{128, 0, 128, "Purple"},
	{238, 130, 238, "Violet"},
	{255, 192, 203, "Pink"},
	{0, 0, 0, "Black"},
	{255, 255, 255, "White"},
	{0, 0, 0, "Rainbow"}
};
const int COLOR_COUNT = sizeof(g_ColorPresets) / sizeof(g_ColorPresets[0]);

class CRainbowThinker : public CBaseEntity
{
public:
	DECLARE_CLASS(CRainbowThinker, CBaseEntity);
	DECLARE_DATADESC();

	EHANDLE m_hTargetEnt;

	void Spawn()
	{
		SetNextThink(gpGlobals->curtime + 0.05f);
	}

	void Think()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			float t = gpGlobals->curtime * 2.0f;
			int r = static_cast<int>(sin(t + 0.0f) * 127 + 128);
			int g = static_cast<int>(sin(t + 2.0f) * 127 + 128);
			int b = static_cast<int>(sin(t + 4.0f) * 127 + 128);
			pTarget->SetRenderColor(r, g, b);
			SetNextThink(gpGlobals->curtime + 0.05f);
		}
		else
		{
			UTIL_Remove(this);
		}
	}
};

LINK_ENTITY_TO_CLASS(env_rainbowthink, CRainbowThinker);

BEGIN_DATADESC(CRainbowThinker)
	DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
	DEFINE_FUNCTION(Think),
END_DATADESC()

static Vector GetDuplicateSpawnPos( CBasePlayer *pPlayer )
{
	Vector forward;
	pPlayer->EyeVectors( &forward );
	return pPlayer->EyePosition() + forward * 96.0f;
}

class CWeaponMultitool : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponMultitool, CBaseHL2MPCombatWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CWeaponMultitool();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual void ItemPostFrame();

private:
	CBaseEntity* FindEntityInFront();
	void ApplyToolAction(CBaseEntity* pEnt);
	void ShowDistance(CBaseEntity* pEnt);

	int m_nMultitoolMode;
	int m_nColorIndex;
	int m_nDecalIndex;
	EHANDLE m_hConstraintTarget1;
	EHANDLE m_hConstraintTarget2;
	int m_nConstraintType; // 0: BallSocket, 1: Weld, 2: Axis

	struct DupedEntityData_t
	{
		string_t className;
		string_t modelName;
		QAngle   angles;
		color32  color;
		//int      skin;
		//int      body;
		float    mass;
		bool     valid;
	};

	DupedEntityData_t m_DupeData;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponMultitool, DT_WeaponMultitool)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_multitool, CWeaponMultitool);
PRECACHE_WEAPON_REGISTER(weapon_multitool);

BEGIN_DATADESC(CWeaponMultitool)
	DEFINE_FIELD(m_hConstraintTarget1, FIELD_EHANDLE),
	DEFINE_FIELD(m_hConstraintTarget2, FIELD_EHANDLE),
	DEFINE_FIELD(m_nConstraintType, FIELD_INTEGER),
	DEFINE_FIELD(m_nMultitoolMode, FIELD_INTEGER),
	DEFINE_FIELD(m_nColorIndex, FIELD_INTEGER),
END_DATADESC()

CWeaponMultitool::CWeaponMultitool()
{
	m_nMultitoolMode = 0;
	m_nColorIndex = 0;
	m_nDecalIndex = 0;
	m_hConstraintTarget1 = NULL;
	m_hConstraintTarget2 = NULL;
	m_nConstraintType = 0;
	m_DupeData.valid = false;
}

enum MultitoolMode_t
{
	MODE_REMOVE = 0,
	MODE_DISTANCE,
	MODE_COLOR,
	MODE_CONSTRAINT,
	MODE_IGNITE,
	MODE_DUPLICATE,
	MODE_MAX
};

const char* g_szModeNames[] =
{
	"Remove",
	"Distance (Source Engine Units)",
	"Color",
	"Constraints",
	"Ignite",
	"Duplicate"
};

const char* g_szConstraintTypes[] = {
	"BallSocket",
	"Weld",
	"Rope"
};
const int CONSTRAINT_TYPE_COUNT = sizeof(g_szConstraintTypes) / sizeof(g_szConstraintTypes[0]);

CBaseEntity* CWeaponMultitool::FindEntityInFront()
{
	trace_t tr;
	Vector start = GetOwner()->EyePosition();
	Vector forward;
	ToBasePlayer(GetOwner())->EyeVectors(&forward);

	UTIL_TraceLine(start, start + forward * 2000, MASK_SOLID, GetOwner(), COLLISION_GROUP_NONE, &tr);
	return tr.m_pEnt;
}

void CWeaponMultitool::PrimaryAttack()
{
	CBaseEntity* pEnt = FindEntityInFront();
	if (!pEnt || pEnt == GetOwner())
		return;

	ApplyToolAction(pEnt);

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	EmitSound("Weapon_357.Single");

		trace_t tr;
	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	pPlayer->EyeVectors(&vecForward);
	Vector vecEnd = vecStart + vecForward * 2048;

	UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction < 1.0f)
	{
		g_pEffects->Sparks(tr.endpos, 4, 2);
	}

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
}

void CWeaponMultitool::SecondaryAttack()
{
	m_nMultitoolMode = (m_nMultitoolMode + 1) % MODE_MAX;

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
		HudText(pPlayer, UTIL_VarArgs("Multitool: %s", g_szModeNames[m_nMultitoolMode]), 255, 255, 200);

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;
}

void RainbowThink(CBaseEntity* pEnt)
{
	if (!pEnt)
		return;

	float t = gpGlobals->curtime * 2.0f;
	int r = static_cast<int>(sin(t + 0.0f) * 127 + 128);
	int g = static_cast<int>(sin(t + 2.0f) * 127 + 128);
	int b = static_cast<int>(sin(t + 4.0f) * 127 + 128);

	pEnt->SetRenderColor(r, g, b);
	pEnt->SetNextThink(gpGlobals->curtime + 0.05f);
}

void CWeaponMultitool::ApplyToolAction(CBaseEntity* pEnt)
{
		CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	switch (m_nMultitoolMode)
	{
	case MODE_REMOVE:
	{
		if (!pEnt->IsWorld())
			UTIL_Remove(pEnt);
		break;
	}
	case MODE_DISTANCE:
	{
		ShowDistance(pEnt);
		break;
	}

		case MODE_COLOR:
		{
			if (m_nColorIndex == COLOR_COUNT - 1) // Rainbow
			{
				
				CBaseEntity* pChild = NULL;
				while ((pChild = gEntList.FindEntityByClassname(pChild, "env_rainbowthink")) != NULL)
				{
					CRainbowThinker* pThinker = dynamic_cast<CRainbowThinker*>(pChild);
					if (pThinker && pThinker->m_hTargetEnt == pEnt)
					{
						UTIL_Remove(pThinker);
					}
				}

				
				CRainbowThinker* pThink = static_cast<CRainbowThinker*>(CreateEntityByName("env_rainbowthink"));
				if (pThink)
				{
					pThink->m_hTargetEnt = pEnt;
					pThink->Spawn();
					DispatchSpawn(pThink);
				}
			}
			else
			{
				
				CBaseEntity* pChild = NULL;
				while ((pChild = gEntList.FindEntityByClassname(pChild, "env_rainbowthink")) != NULL)
				{
					CRainbowThinker* pThinker = dynamic_cast<CRainbowThinker*>(pChild);
					if (pThinker && pThinker->m_hTargetEnt == pEnt)
					{
						UTIL_Remove(pThinker);
					}
				}

				const ColorPreset& color = g_ColorPresets[m_nColorIndex];
				pEnt->SetRenderColor(color.r, color.g, color.b);
			}
			break;
		}

case MODE_CONSTRAINT:
{
	if (!m_hConstraintTarget1)
	{
		m_hConstraintTarget1 = pEnt;
		m_hConstraintTarget2 = NULL; // Reset second just in case
		HudText(pPlayer, "First point selected.", 200, 255, 255);
	}
	else if (m_hConstraintTarget1 == pEnt)
	{
		HudText(pPlayer, "Cannot constrain an entity to itself.", 255, 100, 100);
		m_hConstraintTarget1 = NULL;
	}
	else
	{
		m_hConstraintTarget2 = pEnt;

		CBaseEntity* pEnt1 = m_hConstraintTarget1;
		CBaseEntity* pEnt2 = m_hConstraintTarget2;

		Vector origin1 = pEnt1->WorldSpaceCenter();
		Vector origin2 = pEnt2->WorldSpaceCenter();
		Vector anchor = (origin1 + origin2) * 0.5f;

		IPhysicsObject* pPhys1 = pEnt1->VPhysicsGetObject();
		IPhysicsObject* pPhys2 = pEnt2->VPhysicsGetObject();

		if (!pPhys1 || !pPhys2)
		{
			HudText(pPlayer, "Both objects must have physics!", 255, 100, 100);
			m_hConstraintTarget1 = NULL;
			m_hConstraintTarget2 = NULL;
			break;
		}

		switch (m_nConstraintType)
		{
		case 0: // BallSocket
		{
			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
			ballsocket.InitWithCurrentObjectState(pPhys1, pPhys2, anchor);
			IPhysicsConstraint* pConstraint = physenv->CreateBallsocketConstraint(pPhys1, pPhys2, NULL, ballsocket);
			if (pConstraint)
				HudText(pPlayer, "BallSocket constraint created.", 100, 255, 100);
			break;
		}
		case 1: // Weld
		{
			constraint_fixedparams_t weld;
			weld.Defaults();
			weld.InitWithCurrentObjectState(pPhys1, pPhys2);
			IPhysicsConstraint* pConstraint = physenv->CreateFixedConstraint(pPhys1, pPhys2, NULL, weld);
			if (pConstraint)
				HudText(pPlayer, "Weld constraint created.", 100, 255, 100);
			break;
		}
		case 2: // Rope
		{
			PrecacheModel("cable/cable.vmt");
			CRopeKeyframe* pRope = CRopeKeyframe::Create(pEnt1, pEnt2, 0, 0, 2, "cable/cable.vmt", 10);
			if (pRope)
			{
				constraint_lengthparams_t rope;
				rope.Defaults();
				rope.InitWorldspace(pPhys1, pPhys2, origin1, origin2, false);
				rope.minLength = 0.0f;
				rope.totalLength = (origin1 - origin2).Length();
				IPhysicsConstraint* pConstraint = physenv->CreateLengthConstraint(pPhys1, pPhys2, NULL, rope);
				if (pConstraint)
					HudText(pPlayer, "Rope constraint created.", 100, 255, 100);
				else
					HudText(pPlayer, "Rope visual OK, physics constraint failed.", 255, 200, 100);
			}
			else
			{
				HudText(pPlayer, "Failed to create visual rope.", 255, 100, 100);
			}
			break;
		}
		}

		m_hConstraintTarget1 = NULL;
		m_hConstraintTarget2 = NULL;
	}
	break;
}
	case MODE_IGNITE:
	{
		if ( pEnt->IsWorld() )
		{
			HudText( pPlayer, "Cannot ignite the world.", 255, 100, 100 );
			break;
		}

		// Check if entity already has a flame attached
		CEntityFlame *pFlame = dynamic_cast<CEntityFlame*>(
			gEntList.FindEntityByClassname( NULL, "entityflame" )
		);

		bool bAlreadyOnFire = false;

		while ( pFlame )
		{
			if ( pFlame->GetMoveParent() == pEnt )
			{
				bAlreadyOnFire = true;
				break;
			}
			pFlame = dynamic_cast<CEntityFlame*>(
				gEntList.FindEntityByClassname( pFlame, "entityflame" )
			);
		}

		if ( bAlreadyOnFire )
		{
			// Extinguish = remove flame entity
			UTIL_Remove( pFlame );
			HudText( pPlayer, "Fire extinguished.", 150, 200, 255 );
		}
		else
		{
			// Ignite by attaching an entityflame
			CEntityFlame *pNewFlame = CEntityFlame::Create( pEnt );
			if ( pNewFlame )
			{
				pNewFlame->SetLifetime( 10.0f ); // seconds
				HudText( pPlayer, "Entity ignited.", 255, 150, 50 );
			}
		}

		break;
	}
case MODE_DUPLICATE:
{
	if ( !m_DupeData.valid )
	{
		HudText( pPlayer, "No copied object. Shift+R to copy.", 255, 150, 100 );
		break;
	}

	trace_t tr;
	Vector start = pPlayer->EyePosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );

	Vector end = start + forward * 4096.0f;

	UTIL_TraceLine(
		start,
		end,
		MASK_SOLID,
		pPlayer,
		COLLISION_GROUP_NONE,
		&tr
	);

	if ( tr.fraction == 1.0f )
	{
		HudText( pPlayer, "Aim at a surface to place.", 255, 100, 100 );
		break;
	}

	Vector spawnPos = tr.endpos + tr.plane.normal * 8.0f;

	CBaseEntity *pNew = CreateEntityByName( STRING( m_DupeData.className ) );
	if ( !pNew )
	{
		HudText( pPlayer, "Spawn failed.", 255, 100, 100 );
		break;
	}

	pNew->SetAbsOrigin( spawnPos );
	pNew->SetAbsAngles( m_DupeData.angles );

	if ( m_DupeData.modelName != NULL_STRING )
		pNew->SetModel( STRING( m_DupeData.modelName ) );

	pNew->SetRenderColor(
		m_DupeData.color.r,
		m_DupeData.color.g,
		m_DupeData.color.b
	);

	//pNew->m_nSkin = m_DupeData.skin;
	//pNew->m_nBody = m_DupeData.body;

	DispatchSpawn( pNew );
	pNew->Activate();

	IPhysicsObject *pPhys = pNew->VPhysicsGetObject();
	if ( pPhys && m_DupeData.mass > 0.0f )
	{
		pPhys->SetMass( m_DupeData.mass );
		pPhys->Wake();
	}

	HudText( pPlayer, "Duplicate placed.", 150, 255, 150 );
	break;
}

}
}

void CWeaponMultitool::ShowDistance(CBaseEntity* pEnt)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	float flDist = (pEnt->WorldSpaceCenter() - pPlayer->EyePosition()).Length();
	HudText(pPlayer, UTIL_VarArgs("Distance: %.1f units", flDist), 200, 255, 200);
}

void CWeaponMultitool::ItemPostFrame()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	if (m_nMultitoolMode == MODE_COLOR)
	{
		if ((pPlayer->m_nButtons & IN_RELOAD) && (pPlayer->m_afButtonPressed & IN_RELOAD))
		{
			if (m_nMultitoolMode == MODE_COLOR)
			{
				m_nColorIndex = (m_nColorIndex + 1) % COLOR_COUNT;
				const char* name = g_ColorPresets[m_nColorIndex].name;
				HudText(pPlayer, UTIL_VarArgs("Color Mode: %s", name), 255, 255, 255);
			}
		}
	}
	if (m_nMultitoolMode == MODE_CONSTRAINT)
{
	if ((pPlayer->m_nButtons & IN_RELOAD) && (pPlayer->m_afButtonPressed & IN_RELOAD))
	{
		m_nConstraintType = (m_nConstraintType + 1) % CONSTRAINT_TYPE_COUNT;
		HudText(pPlayer, UTIL_VarArgs("Constraint Type: %s", g_szConstraintTypes[m_nConstraintType]), 255, 255, 255);
	}

	if ((pPlayer->m_nButtons & IN_USE) && (pPlayer->m_afButtonPressed & IN_USE))
	{
		int removed = 0;
		CBaseEntity* pEnt = NULL;

		// Remove ropes
		while ((pEnt = gEntList.FindEntityByClassname(pEnt, "move_rope")) != NULL)
		{
			UTIL_Remove(pEnt);
			removed++;
		}
		pEnt = NULL;
		while ((pEnt = gEntList.FindEntityByClassname(pEnt, "keyframe_rope")) != NULL)
		{
			UTIL_Remove(pEnt);
			removed++;
		}

		// Remove physics constraints
		pEnt = NULL;
		while ((pEnt = gEntList.FindEntityByClassname(pEnt, "phys_constraint")) != NULL)
		{
			UTIL_Remove(pEnt);
			removed++;
		}
		pEnt = NULL;
		while ((pEnt = gEntList.FindEntityByClassname(pEnt, "phys_constraintsystem")) != NULL)
		{
			UTIL_Remove(pEnt);
			removed++;
		}

		HudText(pPlayer, UTIL_VarArgs("Removed %d constraints.", removed), 255, 100, 100);
	}
}
	if ( (pPlayer->m_afButtonPressed & IN_RELOAD) &&
		(pPlayer->m_nButtons & IN_SPEED) )
	{
		if ( m_nMultitoolMode == MODE_DUPLICATE )
		{
				CBaseEntity *pEnt = FindEntityInFront();
				if ( !pEnt || pEnt->IsWorld() )
				{
					HudText( pPlayer, "Nothing to copy.", 255, 100, 100 );
					return;
				}

				m_DupeData.className = AllocPooledString( pEnt->GetClassname() );
				m_DupeData.modelName = pEnt->GetModelName();
				m_DupeData.angles    = pEnt->GetAbsAngles();
				m_DupeData.color     = pEnt->GetRenderColor();

				m_DupeData.mass = 0.0f;
				IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
				if ( pPhys )
					m_DupeData.mass = pPhys->GetMass();

				m_DupeData.valid = true;

				HudText(
				pPlayer,
				UTIL_VarArgs(
					"Object copied: Classname %s with model %s",
					STRING( m_DupeData.className ),
					STRING( m_DupeData.modelName )
				),
				150, 255, 150
			);


		}
	}

	BaseClass::ItemPostFrame();
}
