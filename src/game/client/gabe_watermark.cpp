#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Panel.h"

// Memdbgon.h MUST be the last include in a .cpp file!!!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern IClientMode *g_pClientMode;

#define FONTFLAG_ANTIALIAS 0x010

ConVar gabe_watermark_r( "gabe+_watermark_r", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Red." );
ConVar gabe_watermark_g( "gabe+_watermark_g", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Green." );
ConVar gabe_watermark_b( "gabe+_watermark_b", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Blue." );
ConVar gabe_watermark_a( "gabe+_watermark_a", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark alpha." );

// ----------------------------------------------------------------------

class CHudDMWatermark : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE( CHudDMWatermark, Panel );

    CHudDMWatermark( const char *pElementName );

    virtual void ApplySchemeSettings( IScheme *pScheme );
    virtual bool ShouldDraw( void );
    virtual void Paint( void );

private:
    HFont m_hFontTitle;
    HFont m_hFontHint;
};

DECLARE_HUDELEMENT( CHudDMWatermark );

CHudDMWatermark::CHudDMWatermark( const char *pElementName ) 
    : CHudElement( pElementName ), Panel( NULL, "HudDMWatermark" )
{
    SetParent( g_pClientMode->GetViewport() );
    SetHiddenBits( 0 );

    int w, h;
    surface()->GetScreenSize( w, h );
    SetPos( 0, 0 );
    SetSize( w, h );
}

void CHudDMWatermark::ApplySchemeSettings( IScheme *pScheme )
{
    BaseClass::ApplySchemeSettings( pScheme );

    m_hFontTitle = pScheme->GetFont( "Tahoma", true );
    if ( !m_hFontTitle )
    {
        m_hFontTitle = surface()->CreateFont();
        surface()->SetFontGlyphSet( m_hFontTitle, "Tahoma", 24, 700, 0, 0, FONTFLAG_ANTIALIAS );
    }

    m_hFontHint = pScheme->GetFont( "Verdana", true );
    if ( !m_hFontHint )
    {
        m_hFontHint = surface()->CreateFont();
        surface()->SetFontGlyphSet( m_hFontHint, "Verdana", 16, 500, 0, 0, FONTFLAG_ANTIALIAS );
    }

    int w, h;
    surface()->GetScreenSize( w, h );
    SetPos( 0, 0 );
    SetSize( w, h );
}

bool CHudDMWatermark::ShouldDraw( void )
{
    return true;
}

void CHudDMWatermark::Paint( void )
{
    int screenW, screenH;
    surface()->GetScreenSize( screenW, screenH );

    wchar_t wszTitle[256];
    g_pVGuiLocalize->ConvertANSIToUnicode( "Gabe Mod +", wszTitle, sizeof( wszTitle ) );

    wchar_t wszHint[256] = L"";
    V_wcsncpy(
        wszHint,
        L"sites.google.com/view/pmbruggaming",
        sizeof( wszHint )
    );

    int r = gabe_watermark_r.GetInt();
    int g = gabe_watermark_g.GetInt();
    int b = gabe_watermark_b.GetInt();
    int a = gabe_watermark_a.GetInt();

    int titleW, titleH;
    surface()->GetTextSize( m_hFontTitle, wszTitle, titleW, titleH );

    int hintW = 0, hintH = 0;
    if ( wcslen( wszHint ) > 0 )
        surface()->GetTextSize( m_hFontHint, wszHint, hintW, hintH );

    const int padding = 6;
    const int fadeSize = 8;

    int boxW = max( titleW, hintW ) + padding * 2;
    int boxH = titleH + ( hintH ? hintH + 2 : 0 ) + padding * 2;

    int x = screenW - boxW - 10;
    int y = 10;

    // --------------------------------------------------
    // Fade-out edges (UNCHANGED)
    // --------------------------------------------------
    for ( int i = fadeSize; i > 0; --i )
    {
        int alpha = ( a * i ) / ( fadeSize * 2 );

        surface()->DrawSetColor( 0, 0, 0, alpha );
        surface()->DrawFilledRect(
            x - i,
            y - i,
            x + boxW + i,
            y + boxH + i
        );
    }

    // --------------------------------------------------
    // Solid center box (UNCHANGED)
    // --------------------------------------------------
    surface()->DrawSetColor( 0, 0, 0, a );
    surface()->DrawFilledRect(
        x,
        y,
        x + boxW,
        y + boxH
    );

    // --------------------------------------------------
    // RIGHT-ALIGNED TEXT POSITIONS
    // --------------------------------------------------
    int titleX = x + boxW - padding - titleW;
    int titleY = y + padding;

    int hintX  = x + boxW - padding - hintW;
    int hintY  = titleY + titleH + 2;

    // --------------------------------------------------
    // Title
    // --------------------------------------------------
    surface()->DrawSetTextFont( m_hFontTitle );
    surface()->DrawSetTextColor( r, g, b, a );
    surface()->DrawSetTextPos( titleX, titleY );
    surface()->DrawPrintText( wszTitle, wcslen( wszTitle ) );

    // --------------------------------------------------
    // Hint
    // --------------------------------------------------
    if ( wcslen( wszHint ) > 0 )
    {
        surface()->DrawSetTextFont( m_hFontHint );
        surface()->DrawSetTextColor( r, g, b, a );
        surface()->DrawSetTextPos( hintX, hintY );
		vgui::surface()->DrawPrintText( wszHint, wcslen( wszHint ) );
    }
}
