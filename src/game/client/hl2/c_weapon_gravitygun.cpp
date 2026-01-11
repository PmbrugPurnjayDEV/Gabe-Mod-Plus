//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "hud.h"
#include "in_buttons.h"
#include "beamdraw.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "ClientEffectPrecacheSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar physgun_beamtex(
    "gabe+_physgun_beamtex",
    "sprites/physbeam",
    FCVAR_ARCHIVE | FCVAR_REPLICATED,
    "Material/texture for the physgun beam"
);

static const char* kPhysgunTipMaterial = "sprites/light_glow02_add.vmt";
static const float  kPhysgunTipSize    = 36.0f; // Units

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGravityGun )
CLIENTEFFECT_MATERIAL( physgun_beamtex.GetString() )
CLIENTEFFECT_REGISTER_END()

class C_BeamQuadratic : public CDefaultClientRenderable
{
public:
	C_BeamQuadratic();
	void			Update( C_BaseEntity *pOwner );

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return m_worldPosition; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual bool					ShouldDraw( void ) { return true; }
	virtual bool					IsTransparent( void ) { return true; }
	virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; }
	virtual int						DrawModel( int flags );

	// In C_BeamQuadratic
	virtual void GetRenderBounds( Vector& mins, Vector& maxs )
	{
		// Compute bounds relative to origin
		Vector localTarget = m_targetPosition - m_worldPosition;

		mins.Init(
			min( 0.0f, localTarget.x ) - 8.0f,
			min( 0.0f, localTarget.y ) - 8.0f,
			min( 0.0f, localTarget.z ) - 8.0f
		);
		maxs.Init(
			max( 0.0f, localTarget.x ) + 8.0f,
			max( 0.0f, localTarget.y ) + 8.0f,
			max( 0.0f, localTarget.z ) + 8.0f
		);
	}

	virtual void GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
	{
		Vector start = m_worldPosition;
		Vector end   = m_targetPosition;

		mins.x = min( start.x, end.x ) - 8.0f;
		mins.y = min( start.y, end.y ) - 8.0f;
		mins.z = min( start.z, end.z ) - 8.0f;

		maxs.x = max( start.x, end.x ) + 8.0f;
		maxs.y = max( start.y, end.y ) + 8.0f;
		maxs.z = max( start.z, end.z ) + 8.0f;
	}

	C_BaseEntity		   *m_pOwner;
	Vector					m_targetPosition;
	Vector					m_worldPosition;
	int						m_active;
	int						m_glueTouching;
	int						m_viewModelIndex;

	matrix3x4_t z; // this is the struct for renders
	const matrix3x4_t& RenderableToWorldTransform() { return z; }
};


class C_WeaponGravityGun : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS( C_WeaponGravityGun, C_BaseHLCombatWeapon );
public:
	C_WeaponGravityGun() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		if ( gHUD.m_iKeyBits & IN_ATTACK )
		{
			switch ( keynum )
			{
			case MOUSE_WHEEL_UP:
				gHUD.m_iKeyBits |= IN_WEAPON1;
				return 0;

			case MOUSE_WHEEL_DOWN:
				gHUD.m_iKeyBits |= IN_WEAPON2;
				return 0;
			}
		}

		// Allow engine to process
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
	}

	void OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );
		m_beam.Update( this );
	}

private:
	C_WeaponGravityGun( const C_WeaponGravityGun & );

	C_BeamQuadratic	m_beam;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_physgun, C_WeaponGravityGun );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponGravityGun, DT_WeaponGravityGun, CWeaponGravityGun )
	RecvPropVector( RECVINFO_NAME(m_beam.m_targetPosition,m_targetPosition) ),
	RecvPropVector( RECVINFO_NAME(m_beam.m_worldPosition, m_worldPosition) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_active, m_active) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_glueTouching, m_glueTouching) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_viewModelIndex, m_viewModelIndex) ),
END_RECV_TABLE()


C_BeamQuadratic::C_BeamQuadratic()
{
	m_pOwner = NULL;
}

void C_BeamQuadratic::Update( C_BaseEntity *pOwner )
{
	m_pOwner = pOwner;
	if ( m_active )
	{
		if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->AddRenderable( this, RENDER_GROUP_TRANSLUCENT_ENTITY );
		}
		else
		{
			ClientLeafSystem()->RenderableChanged( m_hRenderHandle );
		}
	}
	else if ( !m_active && m_hRenderHandle != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_hRenderHandle );
	}
}

ConVar physgun_beamcolor( 
    "gabe+_physgun_beamcolor", 
    "255 255 255",
    FCVAR_ARCHIVE,
    "RGB color for the physgun beam (r, g, b, 0–255)"
);

ConVar physgun_beam_rainbow( 
    "gabe+_physgun_rainbowbeam", 
    "0",
    FCVAR_ARCHIVE | FCVAR_REPLICATED, 
	"For the physics gun: 1 = rainbow beam. 0 = not rainbow beam." 
);

ConVar physgun_beamwidth( 
    "gabe+_physgun_beamwidth", 
    "6.5", 
    FCVAR_ARCHIVE | FCVAR_REPLICATED,
	"Width of the beam (In Source Engine Units)" 
);

int C_BeamQuadratic::DrawModel( int )
{
	CMatRenderContextPtr pRenderContext( materials );

    Vector points[3];
    Vector color;
    QAngle tmpAngle;

    if ( !m_active )
        return 0;

    C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_viewModelIndex );
    if ( !pEnt )
        return 0;
    pEnt->GetAttachment( 1, points[0], tmpAngle );

    points[1] = 0.5f * ( m_targetPosition + points[0] );
    points[2] = m_worldPosition;

    IMaterial *pMat = materials->FindMaterial( physgun_beamtex.GetString(), TEXTURE_GROUP_CLIENT_EFFECTS );

    if ( physgun_beam_rainbow.GetBool() )
    {
        // --- rainbow ---
        const float t = gpGlobals->curtime * 1.5f;
        const float k = 2.094395102f; // 120°
        float r = 0.5f + 0.5f * sinf( t + 0.0f );
        float g = 0.5f + 0.5f * sinf( t + k );
        float b = 0.5f + 0.5f * sinf( t + 2.0f * k );
        color.Init( r, g, b );
        // ---------------
    }
	else
	{
		int r = 0, g = 0, b = 0;
		sscanf( physgun_beamcolor.GetString(), "%d %d %d", &r, &g, &b );

		// Clamp to safe range
		r = clamp( r, 0, 255 );
		g = clamp( g, 0, 255 );
		b = clamp( b, 0, 255 );

		// Convert to 0–1 float
		color.Init( r / 255.0f, g / 255.0f, b / 255.0f );
	}

    float scrollOffset = gpGlobals->curtime - (int)gpGlobals->curtime;
    pRenderContext->Bind( pMat );
    DrawBeamQuadratic( points[0], points[1], points[2], physgun_beamwidth.GetFloat(), color, scrollOffset );

	// sprite
	IMaterial *pTipMat = materials->FindMaterial( kPhysgunTipMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	pRenderContext->Bind( pTipMat );

	// Convert float color -> color32
	color32 tipCol;
	tipCol.r = (unsigned char)clamp(color.x * 255.0f, 0.0f, 255.0f);
	tipCol.g = (unsigned char)clamp(color.y * 255.0f, 0.0f, 255.0f);
	tipCol.b = (unsigned char)clamp(color.z * 255.0f, 0.0f, 255.0f);
	tipCol.a = 255;

	// Place at beam tip; nudge along beam dir to avoid z-fighting on surfaces
	Vector tipPos = points[2];
	Vector dir = points[2] - points[1];
	VectorNormalize(dir);
	tipPos += dir * 1.0f;

	DrawSprite( tipPos, kPhysgunTipSize, kPhysgunTipSize, tipCol );
    return 1;
}