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
/*
=======================================================================

COMFORT OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"

#define VR_X_POS		360

#define ID_COMFORTVIGNETTE		127
#define ID_HEIGHTADJUST			128
#define ID_ROLLHIT			    129
#define ID_HAPTICINTENSITY	    130
#define ID_HUDDEPTH			    131
#define ID_HUDYOFFSET		    132

#define ID_BACK					133

#define	NUM_HUDDEPTH			6


typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;

    menuslider_s 		comfortvignette;
    menuslider_s 		heightadjust;
    menuradiobutton_s	rollhit;
	menuslider_s 		hapticintensity;
	menuslider_s        huddepth;
	menuslider_s 		hudyoffset;

	menubitmap_s		back;
} comfort_t;

static comfort_t s_comfort;


static void Comfort_SetMenuItems( void ) {
    s_comfort.comfortvignette.curvalue		= trap_Cvar_VariableValue( "vr_comfortVignette" );
    s_comfort.heightadjust.curvalue		= trap_Cvar_VariableValue( "vr_heightAdjust" );
    s_comfort.rollhit.curvalue		    = trap_Cvar_VariableValue( "vr_rollWhenHit" ) != 0;
	s_comfort.hapticintensity.curvalue		= trap_Cvar_VariableValue( "vr_hapticIntensity" );
	s_comfort.huddepth.curvalue		= (int)trap_Cvar_VariableValue( "vr_hudDepth" ) % NUM_HUDDEPTH;
    s_comfort.hudyoffset.curvalue		    = trap_Cvar_VariableValue( "vr_hudYOffset" ) + 200;
}


static void Comfort_MenuEvent( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
    case ID_COMFORTVIGNETTE:
        trap_Cvar_SetValue( "vr_comfortVignette", s_comfort.comfortvignette.curvalue );
        break;

    case ID_HEIGHTADJUST:
        trap_Cvar_SetValue( "vr_heightAdjust", s_comfort.heightadjust.curvalue );
        break;

    case ID_ROLLHIT:
        trap_Cvar_SetValue( "vr_rollWhenHit", s_comfort.rollhit.curvalue );
        break;

    case ID_HAPTICINTENSITY:
        trap_Cvar_SetValue( "vr_hapticIntensity", s_comfort.hapticintensity.curvalue);
        break;

	case ID_HUDDEPTH:
		trap_Cvar_SetValue( "vr_hudDepth", ((int)s_comfort.huddepth.curvalue % NUM_HUDDEPTH));
		break;

    case ID_HUDYOFFSET:
        trap_Cvar_SetValue( "vr_hudYOffset", s_comfort.hudyoffset.curvalue - 200);
        break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void Comfort_MenuInit( void ) {
	int				y;

	memset( &s_comfort, 0 ,sizeof(comfort_t) );

	Comfort_Cache();

	s_comfort.menu.wrapAround = qtrue;
	s_comfort.menu.fullscreen = qtrue;

	s_comfort.banner.generic.type  = MTYPE_BTEXT;
	s_comfort.banner.generic.x	   = 320;
	s_comfort.banner.generic.y	   = 16;
	s_comfort.banner.string		   = "COMFORT OPTIONS";
	s_comfort.banner.color         = color_white;
	s_comfort.banner.style         = UI_CENTER;

	s_comfort.framel.generic.type  = MTYPE_BITMAP;
	s_comfort.framel.generic.name  = ART_FRAMEL;
	s_comfort.framel.generic.flags = QMF_INACTIVE;
	s_comfort.framel.generic.x	   = 0;
	s_comfort.framel.generic.y	   = 78;
	s_comfort.framel.width  	   = 256;
	s_comfort.framel.height  	   = 329;

	s_comfort.framer.generic.type  = MTYPE_BITMAP;
	s_comfort.framer.generic.name  = ART_FRAMER;
	s_comfort.framer.generic.flags = QMF_INACTIVE;
	s_comfort.framer.generic.x	   = 376;
	s_comfort.framer.generic.y	   = 76;
	s_comfort.framer.width  	   = 256;
	s_comfort.framer.height  	   = 334;

	y = 198;
	s_comfort.comfortvignette.generic.type	     = MTYPE_SLIDER;
	s_comfort.comfortvignette.generic.x			 = VR_X_POS;
    s_comfort.comfortvignette.generic.y			 = y;
	s_comfort.comfortvignette.generic.flags	 	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_comfort.comfortvignette.generic.name	     = "Comfort Vignette:";
	s_comfort.comfortvignette.generic.id 	     = ID_COMFORTVIGNETTE;
	s_comfort.comfortvignette.generic.callback  	= Comfort_MenuEvent;
	s_comfort.comfortvignette.minvalue		     = 0.0f;
	s_comfort.comfortvignette.maxvalue		     = 1.0f;

	y += BIGCHAR_HEIGHT+2;
	s_comfort.heightadjust.generic.type	     = MTYPE_SLIDER;
	s_comfort.heightadjust.generic.x			 = VR_X_POS;
    s_comfort.heightadjust.generic.y			 = y;
	s_comfort.heightadjust.generic.flags	 	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_comfort.heightadjust.generic.name	     = "Height Adjust:";
	s_comfort.heightadjust.generic.id 	     = ID_HEIGHTADJUST;
	s_comfort.heightadjust.generic.callback  	= Comfort_MenuEvent;
	s_comfort.heightadjust.minvalue		     = 0.0f;
	s_comfort.heightadjust.maxvalue		     = 1.0f;

    y += BIGCHAR_HEIGHT+2;
    s_comfort.rollhit.generic.type        = MTYPE_RADIOBUTTON;
    s_comfort.rollhit.generic.name	      = "Roll When Hit:";
    s_comfort.rollhit.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_comfort.rollhit.generic.callback    = Comfort_MenuEvent;
    s_comfort.rollhit.generic.id          = ID_ROLLHIT;
    s_comfort.rollhit.generic.x	          = VR_X_POS;
    s_comfort.rollhit.generic.y	          = y;

	y += BIGCHAR_HEIGHT+2;
	s_comfort.hapticintensity.generic.type	     = MTYPE_SLIDER;
	s_comfort.hapticintensity.generic.x			 = VR_X_POS;
	s_comfort.hapticintensity.generic.y			 = y;
	s_comfort.hapticintensity.generic.flags	 	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_comfort.hapticintensity.generic.name	     = "Haptic Intensity:";
	s_comfort.hapticintensity.generic.id 	     	= ID_HAPTICINTENSITY;
	s_comfort.hapticintensity.generic.callback  	= Comfort_MenuEvent;
	s_comfort.hapticintensity.minvalue		     = 0;
	s_comfort.hapticintensity.maxvalue		     = 1.0;

    y += BIGCHAR_HEIGHT+2;
    s_comfort.huddepth.generic.type		= MTYPE_SLIDER;
    s_comfort.huddepth.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
    s_comfort.huddepth.generic.x			= VR_X_POS;
    s_comfort.huddepth.generic.y			= y;
    s_comfort.huddepth.generic.name		= "HUD Depth:";
    s_comfort.huddepth.generic.callback	= Comfort_MenuEvent;
    s_comfort.huddepth.generic.id		= ID_HUDDEPTH;
	s_comfort.huddepth.minvalue		     = 0;
	s_comfort.huddepth.maxvalue		     = 5;

	y += BIGCHAR_HEIGHT+2;
	s_comfort.hudyoffset.generic.type	     = MTYPE_SLIDER;
	s_comfort.hudyoffset.generic.x			 = VR_X_POS;
	s_comfort.hudyoffset.generic.y			 = y;
	s_comfort.hudyoffset.generic.flags	 	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_comfort.hudyoffset.generic.name	     = "HUD Vertical Position:";
	s_comfort.hudyoffset.generic.id 	     	= ID_HUDYOFFSET;
	s_comfort.hudyoffset.generic.callback  	= Comfort_MenuEvent;
	s_comfort.hudyoffset.minvalue		     = 0;
	s_comfort.hudyoffset.maxvalue		     = 400;

	s_comfort.back.generic.type	    = MTYPE_BITMAP;
	s_comfort.back.generic.name     = ART_BACK0;
	s_comfort.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_comfort.back.generic.callback = Comfort_MenuEvent;
	s_comfort.back.generic.id	    = ID_BACK;
	s_comfort.back.generic.x		= 0;
	s_comfort.back.generic.y		= 480-64;
	s_comfort.back.width  		    = 128;
	s_comfort.back.height  		    = 64;
	s_comfort.back.focuspic         = ART_BACK1;

	Menu_AddItem( &s_comfort.menu, &s_comfort.banner );
	Menu_AddItem( &s_comfort.menu, &s_comfort.framel );
	Menu_AddItem( &s_comfort.menu, &s_comfort.framer );

	Menu_AddItem( &s_comfort.menu, &s_comfort.comfortvignette );
	Menu_AddItem( &s_comfort.menu, &s_comfort.heightadjust );
	Menu_AddItem( &s_comfort.menu, &s_comfort.rollhit );
	Menu_AddItem( &s_comfort.menu, &s_comfort.hapticintensity );
	Menu_AddItem( &s_comfort.menu, &s_comfort.huddepth );
	Menu_AddItem( &s_comfort.menu, &s_comfort.hudyoffset );

	Menu_AddItem( &s_comfort.menu, &s_comfort.back );

	Comfort_SetMenuItems();
}


/*
===============
Comfort_Cache
===============
*/
void Comfort_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_ComfortMenu
===============
*/
void UI_ComfortMenu( void ) {
	Comfort_MenuInit();
	UI_PushMenu( &s_comfort.menu );
}
