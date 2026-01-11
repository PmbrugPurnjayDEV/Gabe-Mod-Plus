#include "cbase.h"
#include "gabe_cvars.h"

CON_COMMAND( gabeplus_setmotd, "Set server MOTD" )
{
	if ( args.ArgC() < 2 )
		return;

	// Everything after the command is the MOTD
	gabeplus_motd.SetValue( args.ArgS() );
}
