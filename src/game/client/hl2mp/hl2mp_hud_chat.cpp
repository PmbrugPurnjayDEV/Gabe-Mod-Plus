//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hl2mp_hud_chat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "ihudlcd.h"



DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

ConVar hud_chat_rainbow(
	"gabeplus_chat_rainbow",
	"0",
	FCVAR_ARCHIVE,
	"Enable rainbow chat colors"
);

static Color HSVtoRGB( float h, float s, float v )
{
	float r, g, b;

	int i = int(h * 6.0f);
	float f = h * 6.0f - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);

	switch (i % 6)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}

	return Color(
		(int)(r * 255),
		(int)(g * 255),
		(int)(b * 255),
		255
	);
}

//=====================
//CHudChatLine
//=====================

void CHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//=====================
//CHudChatInputLine
//=====================

void CHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
	
}

void CHudChat::CreateChatInputLine( void )
{
	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
	m_ChatLine = new CHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );	
}

void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}


void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: Overrides base reset to not cancel chat at round restart
//-----------------------------------------------------------------------------
void CHudChat::Reset( void )
{
}

int CHudChat::GetChatInputOffset( void )
{
	if ( m_pChatInput->IsVisible() )
	{
		return m_iFontHeight;
	}
	else
		return 0;
}

Color CHudChat::GetClientColor( int clientIndex )
{
	// Console messages stay yellow
	if ( clientIndex == 0 )
		return g_ColorYellow;

	// Rainbow override
	if ( hud_chat_rainbow.GetBool() )
	{
		float time = gpGlobals->realtime * 0.2f; // speed
		float hue  = fmod( time + (clientIndex * 0.1f), 1.0f );
		return HSVtoRGB( hue, 1.0f, 1.0f );
	}

	// Normal team coloring
	if ( g_PR )
	{
		switch ( g_PR->GetTeam( clientIndex ) )
		{
		case TEAM_COMBINE: return g_ColorBlue;
		case TEAM_REBELS:  return g_ColorRed;
		default:           return g_ColorYellow;
		}
	}

	return g_ColorYellow;
}
