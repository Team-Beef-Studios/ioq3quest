/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"

void GraphicsOptions_MenuInit( void );

/*
=======================================================================

DRIVER INFORMATION MENU

=======================================================================
*/


#define DRIVERINFO_FRAMEL	"menu/art/frame2_l"
#define DRIVERINFO_FRAMER	"menu/art/frame1_r"
#define DRIVERINFO_BACK0	"menu/art/back_0"
#define DRIVERINFO_BACK1	"menu/art/back_1"

static char* driverinfo_artlist[] = 
{
	DRIVERINFO_FRAMEL,
	DRIVERINFO_FRAMER,
	DRIVERINFO_BACK0,
	DRIVERINFO_BACK1,
	NULL,
};

#define ID_DRIVERINFOBACK	100

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	framel;
	menubitmap_s	framer;
	char			stringbuff[1024];
	char*			strings[64];
	int				numstrings;
} driverinfo_t;

static driverinfo_t	s_driverinfo;

/*
=================
DriverInfo_Event
=================
*/
static void DriverInfo_Event( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_DRIVERINFOBACK:
			UI_PopMenu();
			break;
	}
}

/*
=================
DriverInfo_MenuDraw
=================
*/
static void DriverInfo_MenuDraw( void )
{
	int	i;
	int	y;

	Menu_Draw( &s_driverinfo.menu );

	UI_DrawString( 320, 80, "VENDOR", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 152, "PIXELFORMAT", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 192, "EXTENSIONS", UI_CENTER|UI_SMALLFONT, color_red );

	UI_DrawString( 320, 80+16, uis.glconfig.vendor_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 96+16, uis.glconfig.version_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 112+16, uis.glconfig.renderer_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 152+16, va ("color(%d-bits) Z(%d-bits) stencil(%d-bits)", uis.glconfig.colorBits, uis.glconfig.depthBits, uis.glconfig.stencilBits), UI_CENTER|UI_SMALLFONT, text_color_normal );

	// double column
	y = 192+16;
	for (i=0; i<s_driverinfo.numstrings/2; i++) {
		UI_DrawString( 320-4, y, s_driverinfo.strings[i*2], UI_RIGHT|UI_SMALLFONT, text_color_normal );
		UI_DrawString( 320+4, y, s_driverinfo.strings[i*2+1], UI_LEFT|UI_SMALLFONT, text_color_normal );
		y += SMALLCHAR_HEIGHT;
	}

	if (s_driverinfo.numstrings & 1)
		UI_DrawString( 320, y, s_driverinfo.strings[s_driverinfo.numstrings-1], UI_CENTER|UI_SMALLFONT, text_color_normal );
}

/*
=================
DriverInfo_Cache
=================
*/
void DriverInfo_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!driverinfo_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(driverinfo_artlist[i]);
	}
}

/*
=================
UI_DriverInfo_Menu
=================
*/
static void UI_DriverInfo_Menu( void )
{
	char*	eptr;
	int		i;
	int		len;

	// zero set all our globals
	memset( &s_driverinfo, 0 ,sizeof(driverinfo_t) );

	DriverInfo_Cache();

	s_driverinfo.menu.fullscreen = qtrue;
	s_driverinfo.menu.draw       = DriverInfo_MenuDraw;

	s_driverinfo.banner.generic.type  = MTYPE_BTEXT;
	s_driverinfo.banner.generic.x	  = 320;
	s_driverinfo.banner.generic.y	  = 16;
	s_driverinfo.banner.string		  = "DRIVER INFO";
	s_driverinfo.banner.color	      = color_white;
	s_driverinfo.banner.style	      = UI_CENTER;

	s_driverinfo.framel.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framel.generic.name  = DRIVERINFO_FRAMEL;
	s_driverinfo.framel.generic.flags = QMF_INACTIVE;
	s_driverinfo.framel.generic.x	  = 0;
	s_driverinfo.framel.generic.y	  = 78;
	s_driverinfo.framel.width  	      = 256;
	s_driverinfo.framel.height  	  = 329;

	s_driverinfo.framer.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framer.generic.name  = DRIVERINFO_FRAMER;
	s_driverinfo.framer.generic.flags = QMF_INACTIVE;
	s_driverinfo.framer.generic.x	  = 376;
	s_driverinfo.framer.generic.y	  = 76;
	s_driverinfo.framer.width  	      = 256;
	s_driverinfo.framer.height  	  = 334;

	s_driverinfo.back.generic.type	   = MTYPE_BITMAP;
	s_driverinfo.back.generic.name     = DRIVERINFO_BACK0;
	s_driverinfo.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_driverinfo.back.generic.callback = DriverInfo_Event;
	s_driverinfo.back.generic.id	   = ID_DRIVERINFOBACK;
	s_driverinfo.back.generic.x		   = 0;
	s_driverinfo.back.generic.y		   = 480-64;
	s_driverinfo.back.width  		   = 128;
	s_driverinfo.back.height  		   = 64;
	s_driverinfo.back.focuspic         = DRIVERINFO_BACK1;

  // TTimo: overflow with particularly long GL extensions (such as the gf3)
  // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=399
  // NOTE: could have pushed the size of stringbuff, but the list is already out of the screen
  // (no matter what your resolution)
  Q_strncpyz(s_driverinfo.stringbuff, uis.glconfig.extensions_string, 1024);

	// build null terminated extension strings
	eptr = s_driverinfo.stringbuff;
	while ( s_driverinfo.numstrings<40 && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ')
			s_driverinfo.strings[s_driverinfo.numstrings++] = eptr;

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	// safety length strings for display
	for (i=0; i<s_driverinfo.numstrings; i++) {
		len = strlen(s_driverinfo.strings[i]);
		if (len > 32) {
			s_driverinfo.strings[i][len-1] = '>';
			s_driverinfo.strings[i][len]   = '\0';
		}
	}

	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.banner );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framel );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framer );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.back );

	UI_PushMenu( &s_driverinfo.menu );
}

/*
=======================================================================

GRAPHICS OPTIONS MENU

=======================================================================
*/

#define GRAPHICSOPTIONS_FRAMEL	"menu/art/frame2_l"
#define GRAPHICSOPTIONS_FRAMER	"menu/art/frame1_r"
#define GRAPHICSOPTIONS_BACK0	"menu/art/back_0"
#define GRAPHICSOPTIONS_BACK1	"menu/art/back_1"
#define GRAPHICSOPTIONS_ACCEPT0	"menu/art/accept_0"
#define GRAPHICSOPTIONS_ACCEPT1	"menu/art/accept_1"

#define ID_BACK2		101
#define ID_DRIVERINFO	102
#define ID_GRAPHICS		103
#define ID_SOUND		104
#define ID_NETWORK		105
#define ID_REFRESHRATE	106
#define ID_RAILGUN		107
#define ID_SHADOWS		108
#define ID_PLAYERSHADOW	109
#define ID_GAMMA		110
#define ID_HIGHQUALITYSKY	111

#define	NUM_REFRESHRATE	4
#define NUM_SHADOWS 3
#define NUM_RAILGUN 2

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menuslider_s	tq;
	menulist_s  	lighting;
	menulist_s  	geometry;
	menutext_s		driverinfo;
    menulist_s		refreshrate;
	menulist_s		railgun;
	menulist_s		shadows;
	menulist_s		playershadow;
	menuslider_s	gamma;
	menuradiobutton_s	highqualitysky;

	menubitmap_s	apply;
	menubitmap_s	back;
} graphicsoptions_t;

typedef struct
{
	int tq;
	int lighting;
	int geometry;
	int refreshrate;
	int railgun;
	int shadows;
	int playershadow;
	float gamma;
	int highqualitysky;
} InitialVideoOptions_s;

static InitialVideoOptions_s	s_ivo;
static graphicsoptions_t		s_graphicsoptions;

/*
=================
GraphicsOptions_GetInitialVideo
=================
*/
static void GraphicsOptions_GetInitialVideo( void )
{
	s_ivo.tq          = s_graphicsoptions.tq.curvalue;
	s_ivo.lighting    = s_graphicsoptions.lighting.curvalue;
	s_ivo.geometry    = s_graphicsoptions.geometry.curvalue;
	s_ivo.refreshrate = s_graphicsoptions.refreshrate.curvalue;
	s_ivo.railgun     = s_graphicsoptions.railgun.curvalue;
	s_ivo.shadows     = s_graphicsoptions.refreshrate.curvalue;
	s_ivo.playershadow	= s_graphicsoptions.playershadow.curvalue;
	s_ivo.gamma       = s_graphicsoptions.gamma.curvalue;
	s_ivo.highqualitysky = s_graphicsoptions.highqualitysky.curvalue;
}

/*
=================
GraphicsOptions_UpdateMenuItems
=================
*/
static void GraphicsOptions_UpdateMenuItems( void )
{
	s_graphicsoptions.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( s_ivo.tq != s_graphicsoptions.tq.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.lighting != s_graphicsoptions.lighting.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.geometry != s_graphicsoptions.geometry.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.gamma != s_graphicsoptions.gamma.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
}

/*
=================
GraphicsOptions_ApplyChanges
=================
*/
static void GraphicsOptions_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "r_picmip", 3 - s_graphicsoptions.tq.curvalue );
	trap_Cvar_SetValue( "r_vertexLight", s_graphicsoptions.lighting.curvalue );

	if ( s_graphicsoptions.geometry.curvalue == 2 )
	{
		trap_Cvar_SetValue( "r_lodBias", -1 );
		trap_Cvar_SetValue( "r_subdivisions", 4 );
	}
	else if ( s_graphicsoptions.geometry.curvalue == 1 )
	{
		trap_Cvar_SetValue( "r_lodBias", 1 );
		trap_Cvar_SetValue( "r_subdivisions", 12 );
	}
	else
	{
		trap_Cvar_SetValue( "r_lodBias", 2 );
		trap_Cvar_SetValue( "r_subdivisions", 80 );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
=================
GraphicsOptions_Event
=================
*/
static void GraphicsOptions_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
	 	return;
	}

	switch( ((menucommon_s*)ptr)->id ) {

	case ID_REFRESHRATE: {
			int refresh;
			switch (s_graphicsoptions.refreshrate.curvalue) {
				case 0:
					refresh = 60;
					break;
				case 1:
					refresh = 72;
					break;
				case 2:
					refresh = 80;
					break;
				case 3:
					refresh = 90;
					break;
				}
			trap_Cvar_SetValue("vr_refreshrate", refresh);
		}
		break;

	case ID_RAILGUN:
		trap_Cvar_SetValue( "cg_oldRail", s_graphicsoptions.railgun.curvalue );
		break;

	case ID_SHADOWS: {
			int shadows;
			switch (s_graphicsoptions.shadows.curvalue) {
				case 0:
					shadows = 0;
					break;
				case 1:
				    shadows = 1;
				    break;
				default:
					shadows = 3;
					break;
				}
			trap_Cvar_SetValue("cg_shadows", shadows);
		}
		break;

	case ID_PLAYERSHADOW: {
			int shadow;
			switch (s_graphicsoptions.playershadow.curvalue) {
				case 0:
					shadow = 0;
					break;
				case 1:
				    shadow = 1;
				    break;
				default:
					shadow = 3;
					break;
				}
			trap_Cvar_SetValue("cg_playerShadow", shadow);
		}
		break;

	case ID_GAMMA:
		trap_Cvar_SetValue( "r_gamma", s_graphicsoptions.gamma.curvalue );
		break;

	case ID_HIGHQUALITYSKY:
		trap_Cvar_SetValue( "r_fastsky", !s_graphicsoptions.highqualitysky.curvalue );
		break;

	case ID_DRIVERINFO:
		UI_DriverInfo_Menu();
		break;

	case ID_BACK2:
		UI_PopMenu();
		break;

	case ID_GRAPHICS:
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;
	}
}


/*
================
GraphicsOptions_TQEvent
================
*/
static void GraphicsOptions_TQEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
	 	return;
	}
	s_graphicsoptions.tq.curvalue = (int)(s_graphicsoptions.tq.curvalue + 0.5);
}


/*
================
GraphicsOptions_MenuDraw
================
*/
void GraphicsOptions_MenuDraw (void)
{
//APSFIX - rework this
	GraphicsOptions_UpdateMenuItems();

	Menu_Draw( &s_graphicsoptions.menu );
}

/*
=================
GraphicsOptions_SetMenuItems
=================
*/
static void GraphicsOptions_SetMenuItems( void )
{
	s_graphicsoptions.tq.curvalue = 3-trap_Cvar_VariableValue( "r_picmip");
	if ( s_graphicsoptions.tq.curvalue < 0 )
	{
		s_graphicsoptions.tq.curvalue = 0;
	}
	else if ( s_graphicsoptions.tq.curvalue > 3 )
	{
		s_graphicsoptions.tq.curvalue = 3;
	}

	int lodbias = trap_Cvar_VariableValue( "r_lodBias" );
	if (lodbias == -1) {
		s_graphicsoptions.geometry.curvalue = 2;
	} else if (lodbias == 1) {
		s_graphicsoptions.geometry.curvalue = 1;
	} else {
		s_graphicsoptions.geometry.curvalue = 0;
	}

	switch ( (int) trap_Cvar_VariableValue( "vr_refreshrate" ) )
	{
		case 60:
			s_graphicsoptions.refreshrate.curvalue = 0;
			break;
		case 72:
			s_graphicsoptions.refreshrate.curvalue = 1;
			break;
		case 80:
			s_graphicsoptions.refreshrate.curvalue = 2;
			break;
		case 90:
			s_graphicsoptions.refreshrate.curvalue = 3;
			break;
	}

	switch ( (int) trap_Cvar_VariableValue( "cg_shadows" ) )
	{
		case 0:
			s_graphicsoptions.shadows.curvalue = 0;
			break;
		case 1:
			s_graphicsoptions.shadows.curvalue = 1;
			break;
		default:
			s_graphicsoptions.shadows.curvalue = 2;
			break;
	}

	switch ( (int) trap_Cvar_VariableValue( "cg_playerShadow" ) )
	{
		case 0:
			s_graphicsoptions.playershadow.curvalue = 0;
			break;
		case 1:
			s_graphicsoptions.playershadow.curvalue = 1;
			break;
		default:
			s_graphicsoptions.playershadow.curvalue = 2;
			break;
	}

	s_graphicsoptions.lighting.curvalue = trap_Cvar_VariableValue( "r_vertexLight" ) != 0;
	s_graphicsoptions.railgun.curvalue	= trap_Cvar_VariableValue( "cg_oldRail" );
	s_graphicsoptions.gamma.curvalue			= trap_Cvar_VariableValue( "r_gamma" );
	s_graphicsoptions.highqualitysky.curvalue	= trap_Cvar_VariableValue( "r_fastsky" ) == 0;
}

/*
================
GraphicsOptions_MenuInit
================
*/
void GraphicsOptions_MenuInit( void )
{

	static const char *lighting_names[] =
	{
		"Lightmap (High)",
		"Vertex (Low)",
		NULL
	};

	static const char *quality_names[] =
	{
		"Low",
		"Medium",
		"High",
		NULL
	};

	static const char *s_refreshrate[] =
	{
		"60",
		"72 (Recommended)",
		"80",
		"90",
		NULL
	};

	static const char *s_shadows[] =
	{
		"None",
		"Low",
		"High",
		NULL
	};

	static const char *s_railgun[] =
	{
		"Q2 Style",
		"Q3 Style",
		NULL
	};

	int y;

	// zero set all our globals
	memset( &s_graphicsoptions, 0 ,sizeof(graphicsoptions_t) );
	
	GraphicsOptions_Cache();

	s_graphicsoptions.menu.wrapAround = qtrue;
	s_graphicsoptions.menu.fullscreen = qtrue;
	s_graphicsoptions.menu.draw       = GraphicsOptions_MenuDraw;

	s_graphicsoptions.banner.generic.type  = MTYPE_BTEXT;
	s_graphicsoptions.banner.generic.x	   = 320;
	s_graphicsoptions.banner.generic.y	   = 16;
	s_graphicsoptions.banner.string  	   = "SYSTEM SETUP";
	s_graphicsoptions.banner.color         = color_white;
	s_graphicsoptions.banner.style         = UI_CENTER;

	s_graphicsoptions.framel.generic.type  = MTYPE_BITMAP;
	s_graphicsoptions.framel.generic.name  = GRAPHICSOPTIONS_FRAMEL;
	s_graphicsoptions.framel.generic.flags = QMF_INACTIVE;
	s_graphicsoptions.framel.generic.x	   = 0;
	s_graphicsoptions.framel.generic.y	   = 78;
	s_graphicsoptions.framel.width  	   = 256;
	s_graphicsoptions.framel.height  	   = 329;

	s_graphicsoptions.framer.generic.type  = MTYPE_BITMAP;
	s_graphicsoptions.framer.generic.name  = GRAPHICSOPTIONS_FRAMER;
	s_graphicsoptions.framer.generic.flags = QMF_INACTIVE;
	s_graphicsoptions.framer.generic.x	   = 376;
	s_graphicsoptions.framer.generic.y	   = 76;
	s_graphicsoptions.framer.width  	   = 256;
	s_graphicsoptions.framer.height  	   = 334;

	s_graphicsoptions.graphics.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.graphics.generic.flags	= QMF_RIGHT_JUSTIFY;
	s_graphicsoptions.graphics.generic.id		= ID_GRAPHICS;
	s_graphicsoptions.graphics.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.graphics.generic.x		= 216;
	s_graphicsoptions.graphics.generic.y		= 256 - 2 * PROP_HEIGHT;
	s_graphicsoptions.graphics.string			= "GRAPHICS";
	s_graphicsoptions.graphics.style			= UI_RIGHT;
	s_graphicsoptions.graphics.color			= color_red;

	s_graphicsoptions.sound.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.sound.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.sound.generic.id			= ID_SOUND;
	s_graphicsoptions.sound.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.sound.generic.x			= 216;
	s_graphicsoptions.sound.generic.y			= 256 - PROP_HEIGHT;
	s_graphicsoptions.sound.string				= "SOUND";
	s_graphicsoptions.sound.style				= UI_RIGHT;
	s_graphicsoptions.sound.color				= color_red;

	s_graphicsoptions.network.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.network.generic.id		= ID_NETWORK;
	s_graphicsoptions.network.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.network.generic.x			= 216;
	s_graphicsoptions.network.generic.y			= 256;
	s_graphicsoptions.network.string			= "NETWORK";
	s_graphicsoptions.network.style				= UI_RIGHT;
	s_graphicsoptions.network.color				= color_red;

	y = 254 - 5 * (BIGCHAR_HEIGHT + 2);

	// references "vr_refreshrate"
	s_graphicsoptions.refreshrate.generic.type		= MTYPE_SPINCONTROL;
	s_graphicsoptions.refreshrate.generic.name		= "Refresh Rate:";
	s_graphicsoptions.refreshrate.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.refreshrate.generic.x			= 400;
	s_graphicsoptions.refreshrate.generic.y			= y;
	s_graphicsoptions.refreshrate.itemnames	        = s_refreshrate;
	s_graphicsoptions.refreshrate.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.refreshrate.generic.id		= ID_REFRESHRATE;
	s_graphicsoptions.refreshrate.numitems			= NUM_REFRESHRATE;
	y += BIGCHAR_HEIGHT+2;

	// references "r_gamma"
	s_graphicsoptions.gamma.generic.type			= MTYPE_SLIDER;
	s_graphicsoptions.gamma.generic.name			= "Brightness:";
	s_graphicsoptions.gamma.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.gamma.generic.x				= 400;
    s_graphicsoptions.gamma.generic.y				= y;
    s_graphicsoptions.gamma.generic.callback		= GraphicsOptions_Event;
	s_graphicsoptions.gamma.generic.id				= ID_GAMMA;
	s_graphicsoptions.gamma.minvalue				= 0.8f;
	s_graphicsoptions.gamma.maxvalue				= 1.2f;
	y += BIGCHAR_HEIGHT+2;

	// references "cg_oldRail"
	s_graphicsoptions.railgun.generic.type     = MTYPE_SPINCONTROL;
	s_graphicsoptions.railgun.generic.name	   = "Railgun Effect:";
	s_graphicsoptions.railgun.generic.flags	   = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.railgun.generic.x	       = 400;
	s_graphicsoptions.railgun.generic.y	       = y;
	s_graphicsoptions.railgun.itemnames	       = s_railgun;
	s_graphicsoptions.railgun.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.railgun.generic.id       = ID_RAILGUN;
	s_graphicsoptions.railgun.numitems         = NUM_RAILGUN;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_vertexLight"
	s_graphicsoptions.lighting.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.lighting.generic.name	 = "Lighting:";
	s_graphicsoptions.lighting.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.lighting.generic.x	 = 400;
	s_graphicsoptions.lighting.generic.y	 = y;
	s_graphicsoptions.lighting.itemnames     = lighting_names;
	y += BIGCHAR_HEIGHT+2;

	// references "cg_shadows"
	s_graphicsoptions.shadows.generic.type		= MTYPE_SPINCONTROL;
	s_graphicsoptions.shadows.generic.name		= "Opponent Shadows:";
	s_graphicsoptions.shadows.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.shadows.generic.x			= 400;
	s_graphicsoptions.shadows.generic.y			= y;
	s_graphicsoptions.shadows.itemnames	        = s_shadows;
	s_graphicsoptions.shadows.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.shadows.generic.id		= ID_SHADOWS;
	s_graphicsoptions.shadows.numitems			= NUM_SHADOWS;
	y += BIGCHAR_HEIGHT+2;

	// references "cg_playerShadow"
	s_graphicsoptions.playershadow.generic.type		= MTYPE_SPINCONTROL;
	s_graphicsoptions.playershadow.generic.name		= "Player Shadow:";
	s_graphicsoptions.playershadow.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.playershadow.generic.x			= 400;
	s_graphicsoptions.playershadow.generic.y			= y;
	s_graphicsoptions.playershadow.itemnames	        = s_shadows;
	s_graphicsoptions.playershadow.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.playershadow.generic.id		= ID_PLAYERSHADOW;
	s_graphicsoptions.playershadow.numitems			= NUM_SHADOWS;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_lodBias" & "subdivisions"
	s_graphicsoptions.geometry.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.geometry.generic.name	 = "Geometric Detail:";
	s_graphicsoptions.geometry.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.geometry.generic.x	 = 400;
	s_graphicsoptions.geometry.generic.y	 = y;
	s_graphicsoptions.geometry.itemnames     = quality_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_picmip"
	s_graphicsoptions.tq.generic.type	= MTYPE_SLIDER;
	s_graphicsoptions.tq.generic.name	= "Texture Detail:";
	s_graphicsoptions.tq.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.tq.generic.x		= 400;
	s_graphicsoptions.tq.generic.y		= y;
	s_graphicsoptions.tq.minvalue       = 0;
	s_graphicsoptions.tq.maxvalue       = 3;
	s_graphicsoptions.tq.generic.callback = GraphicsOptions_TQEvent;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_fastsky"
	s_graphicsoptions.highqualitysky.generic.type     = MTYPE_RADIOBUTTON;
	s_graphicsoptions.highqualitysky.generic.name	  = "High Quality Sky:";
	s_graphicsoptions.highqualitysky.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.highqualitysky.generic.x			= 400;
	s_graphicsoptions.highqualitysky.generic.y			= y;
	s_graphicsoptions.highqualitysky.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.highqualitysky.generic.id       = ID_HIGHQUALITYSKY;
	y += 2*BIGCHAR_HEIGHT;

	s_graphicsoptions.driverinfo.generic.type     = MTYPE_PTEXT;
	s_graphicsoptions.driverinfo.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.driverinfo.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.driverinfo.generic.id       = ID_DRIVERINFO;
	s_graphicsoptions.driverinfo.generic.x        = 320;
	s_graphicsoptions.driverinfo.generic.y        = y;
	s_graphicsoptions.driverinfo.string           = "Driver Info";
	s_graphicsoptions.driverinfo.style            = UI_CENTER|UI_SMALLFONT;
	s_graphicsoptions.driverinfo.color            = color_red;

	s_graphicsoptions.back.generic.type	    = MTYPE_BITMAP;
	s_graphicsoptions.back.generic.name     = GRAPHICSOPTIONS_BACK0;
	s_graphicsoptions.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.back.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.back.generic.id	    = ID_BACK2;
	s_graphicsoptions.back.generic.x		= 0;
	s_graphicsoptions.back.generic.y		= 480-64;
	s_graphicsoptions.back.width  		    = 128;
	s_graphicsoptions.back.height  		    = 64;
	s_graphicsoptions.back.focuspic         = GRAPHICSOPTIONS_BACK1;

	s_graphicsoptions.apply.generic.type     = MTYPE_BITMAP;
	s_graphicsoptions.apply.generic.name     = GRAPHICSOPTIONS_ACCEPT0;
	s_graphicsoptions.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_graphicsoptions.apply.generic.callback = GraphicsOptions_ApplyChanges;
	s_graphicsoptions.apply.generic.x        = 640;
	s_graphicsoptions.apply.generic.y        = 480-64;
	s_graphicsoptions.apply.width  		     = 128;
	s_graphicsoptions.apply.height  		 = 64;
	s_graphicsoptions.apply.focuspic         = GRAPHICSOPTIONS_ACCEPT1;

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.banner );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.framel );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.framer );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.graphics );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.sound );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.network );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.refreshrate );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.railgun );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.gamma );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.lighting );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.shadows );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.playershadow );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.geometry );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.tq );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.highqualitysky );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.driverinfo );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.back );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.apply );

	GraphicsOptions_SetMenuItems();
	GraphicsOptions_GetInitialVideo();
}


/*
=================
GraphicsOptions_Cache
=================
*/
void GraphicsOptions_Cache( void ) {
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_FRAMEL );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_FRAMER );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_BACK0 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_BACK1 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_ACCEPT0 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_ACCEPT1 );
}


/*
=================
UI_GraphicsOptionsMenu
=================
*/
void UI_GraphicsOptionsMenu( void ) {
	GraphicsOptions_MenuInit();
	UI_PushMenu( &s_graphicsoptions.menu );
	Menu_SetCursorToItem( &s_graphicsoptions.menu, &s_graphicsoptions.graphics );
}

