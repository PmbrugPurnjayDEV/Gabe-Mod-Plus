#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/HTML.h>
#include "vgui/IScheme.h"
#include "ienginevgui.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "vgui_int.h"

using namespace vgui;

class CMOTDPanel;

static CMOTDPanel *g_pMOTDPanel = NULL;

class CMOTDPanel : public Frame
{
	DECLARE_CLASS_SIMPLE( CMOTDPanel, Frame );

public:
	CMOTDPanel( VPANEL parent ) : BaseClass( NULL, "MOTDPanel" )
	{
		SetParent( parent );
		SetTitle( "GABE MOD +", true );
		SetSize( 640, 480 );
		SetMoveable( true );
		SetSizeable( false );
		SetCloseButtonVisible( true );
		SetDeleteSelfOnClose( true );
		SetMinimizeButtonVisible( false );
		SetMaximizeButtonVisible( false );

		// Center panel
		int screenWide, screenTall;
		surface()->GetScreenSize( screenWide, screenTall );
		SetPos( (screenWide - 640) / 2, (screenTall - 480) / 2 );

		m_pHTML = new HTML( this, "MOTDHTML" );
		m_pHTML->SetBounds( 10, 30, 620, 440 );
		char fullPath[MAX_PATH];
		Q_snprintf( fullPath, sizeof(fullPath), "file://%s/resource/ui/html/motd.html", engine->GetGameDirectory() );
		m_pHTML->OpenURL( fullPath, NULL );


		MakePopup();
		MoveToFront();
		SetVisible( true );
		Activate();
	}

	virtual void OnClose()
	{
		BaseClass::OnClose();
		g_pMOTDPanel = NULL;
	}

private:
	HTML *m_pHTML;
};

void ShowMOTDPanel()
{
	if ( g_pMOTDPanel )
		return;

	g_pMOTDPanel = new CMOTDPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ) );
}

CON_COMMAND_F( gabeplus_chlog, "Shows the changelog for all to see.", FCVAR_CLIENTDLL )
{
	ShowMOTDPanel();
}
