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

CREDITS

=======================================================================
*/


#include "ui_local.h"

#include "../qcommon/qcommon.h"

typedef struct {
	menuframework_s	menu;
	int frame;
} creditsmenu_t;

static creditsmenu_t	s_credits;


/*
===============
UI_CreditMenu_Draw_ioq3
===============
*/
static void UI_CreditMenu_Draw_ioq3( void ) {
	int		y;
	int		i;

	// These are all people that have made commits to Subversion, and thus
	//  probably incomplete.
	// (These are in alphabetical order, for the defense of everyone's egos.)
	static const char *names[] = {
		"Tim Angus",
		"James Canete",
		"Vincent Cojot",
		"Ryan C. Gordon",
		"Aaron Gyes",
		"Zack Middleton",
		"Ludwig Nussel",
		"Julian Priestley",
		"Scirocco Six",
		"Thilo Schulz",
		"Zachary J. Slater",
		"Tony J. White",
		"...and many, many others!",  // keep this one last.
		NULL
	};

	// Center text vertically on the screen
	y = (SCREEN_HEIGHT - ARRAY_LEN(names) * (1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE)) / 2;

	UI_DrawProportionalString( 320, y, "ioquake3 contributors:", UI_CENTER|UI_SMALLFONT, color_white );
	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;

	for (i = 0; names[i]; i++) {
		UI_DrawProportionalString( 320, y, names[i], UI_CENTER|UI_SMALLFONT, color_white );
		y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	}

	UI_DrawString( 320, 459, "http://www.ioquake3.org/", UI_CENTER|UI_SMALLFONT, color_red );
}


/*
===============
UI_CreditMenu_Draw_ioq3
===============
*/
static void UI_CreditMenu_Draw_q3q( void ) {
	int		y;
	int		i;

/*
Quake3Quest by Team Beef & Friends
-------------------------------------
Lead programmer
Dr Beef

Additional coding
Baggyg, MuaDiB, Lubos, Sparkie

Additional Contributions
Bummser, Skillfur, Ceno, Cukier, Eispfogel, Pizzaluigi

Dedicated Beta Testers
f2hunter, XQuader, Ceno, Cukier, Bummser, Retro1N, Benny91, April, Ikarus,
GeTall, Lubos, MasakaPete, Config2, Maniac, Ghostdog72, Slydog43,
Cornelius, Ferret, RealityForge, PvtGenO, SatanSlayer

Special Thanks to the whole discord!
*/

	y = 12;

	UI_DrawProportionalString( 320, y, "Quake3Quest by Team Beef & Friends", UI_CENTER|UI_SMALLFONT, color_red );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "----------------------------------", UI_CENTER|UI_SMALLFONT, color_white );

    y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    UI_DrawProportionalString( 320, y, "Team Beef are DrBeef,  Baggyg,  Bummser", UI_CENTER|UI_SMALLFONT, color_blue );
    y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Lead programmer", UI_CENTER|UI_SMALLFONT, color_red );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    UI_DrawString( 320, y, "DrBeef", UI_CENTER|UI_SMALLFONT, color_white );

    y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    UI_DrawProportionalString( 320, y, "Companion App      Custom Quest Home", UI_CENTER|UI_SMALLFONT, color_red );
    y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    UI_DrawString( 320, y, "Baggyg                     ROBYER1", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Additional Quake3Quest coding", UI_CENTER|UI_SMALLFONT, color_red );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    UI_DrawString( 320, y, "Sparkie, MuadDib, Lubos, Baggyg", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Additional Contributions", UI_CENTER|UI_SMALLFONT, color_red );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "Bummser, Skillfur, Ceno, Cukier, Eispfogel", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "Omarlego (custom Q3Q background), Pizzaluigi", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Dedicated Beta Testers", UI_CENTER|UI_SMALLFONT, color_red );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "f2hunter, XQuader, Ceno, Cukier, Bummser, Retro1N, Benny91, Madmac(Ikarus),", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "GeTall, Lubos, MasakaPete, Config2, Maniac, Ghostdog72, Slydog43,", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "Myrothas, April, Cornelius, Ferret, RealityForge, PvtGenO, SatanSlayer", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Special Thanks to the whole Team Beef discord!", UI_CENTER|UI_SMALLFONT, color_red );

	qboolean skipIOQ3Credits = trap_Cvar_VariableValue("skip_ioq3_credits") == 1.0f;
	if (skipIOQ3Credits)
	{
		y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
		UI_DrawProportionalString(320, y, "..and additional thanks to all the", UI_CENTER | UI_SMALLFONT, color_orange);
		y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
		UI_DrawProportionalString(320, y, "original ioquake3 contributors!", UI_CENTER | UI_SMALLFONT, color_orange);
	}

	UI_DrawString( 320, 459, "Quake3Quest: https://quake3.quakevr.com/", UI_CENTER|UI_SMALLFONT, color_red );
}


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	qboolean skipIOQ3Credits = trap_Cvar_VariableValue("skip_ioq3_credits") == 1.0f;

	s_credits.frame++;
	if (s_credits.frame == 1) {
		s_credits.menu.draw = UI_CreditMenu_Draw_q3q;
	} else if (s_credits.frame == 2 && !skipIOQ3Credits) {
		//Only show these once the first time someone plays
		trap_Cvar_SetValue("skip_ioq3_credits", 1.0f);
        trap_Cmd_ExecuteText( EXEC_APPEND, "writeconfig " Q3CONFIG_CFG "/n");
        s_credits.menu.draw = UI_CreditMenu_Draw_ioq3;
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
	return 0;
}


/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
	int		y;

	y = 12;
	UI_DrawProportionalString( 320, y, "id Software is:", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Programming", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "John Carmack, Robert A. Duffy, Jim Dose'", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Art", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Adrian Carmack, Kevin Cloud,", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Kenneth Scott, Seneca Menard, Fred Nilsson", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Game Designer", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Graeme Devine", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Level Design", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Tim Willits, Christian Antkow, Paul Jaquays", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "CEO", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Todd Hollenshead", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Director of Business Development", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Marty Stratton", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Biz Assist and id Mom", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Donna Jackson", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Development Assistance", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Eric Webb", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.35 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "Buy the full game on Steam:  https://store.steampowered.com/app/2200/Quake_III_Arena", UI_CENTER|UI_SMALLFONT, color_red );
	y += SMALLCHAR_HEIGHT;
	UI_DrawString( 320, y, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color_red );
}


/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( void ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}
