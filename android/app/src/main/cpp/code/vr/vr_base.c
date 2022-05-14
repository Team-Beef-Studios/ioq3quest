#include "vr_base.h"
#include "../VrApi/Include/VrApi.h"
#include "../VrApi/Include/VrApi_Helpers.h"
#include "../VrApi/Include/VrApi_Types.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

//#if __ANDROID__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#pragma clang diagnostic pop

#include <EGL/egl.h>

#include <assert.h>

static engine_t vr_engine;

cvar_t *vr_worldscale = NULL;
cvar_t *vr_worldscaleScaler = NULL;
cvar_t *vr_hudDepth = NULL;
cvar_t *vr_righthanded = NULL;
cvar_t *vr_switchThumbsticks = NULL;
cvar_t *vr_snapturn = NULL;
cvar_t *vr_heightAdjust = NULL;
cvar_t *vr_extralatencymode = NULL;
cvar_t *vr_directionMode = NULL;
cvar_t *vr_weaponPitch = NULL;
cvar_t *vr_twoHandedWeapons = NULL;
cvar_t *vr_showItemInHand = NULL;
cvar_t *vr_refreshrate = NULL;
cvar_t *vr_weaponScope = NULL;
cvar_t *vr_rollWhenHit = NULL;
cvar_t *vr_hudYOffset = NULL;
cvar_t *vr_sendRollToServer = NULL;
cvar_t *vr_lasersight = NULL;
cvar_t *vr_hapticIntensity = NULL;
cvar_t *vr_comfortVignette = NULL;
cvar_t *vr_weaponSelectorMode = NULL;
cvar_t *vr_weaponSelectorWithHud = NULL;
cvar_t *vr_goreLevel = NULL;
cvar_t *vr_hudDrawStatus = NULL;
cvar_t *vr_showConsoleMessages = NULL;

engine_t* VR_Init( ovrJava java )
{
	ovrInitParms initParams;
	ovrResult initResult;

	memset(&vr_engine, 0, sizeof(vr_engine));

	initParams = vrapi_DefaultInitParms(&java);
	initResult = vrapi_Initialize(&initParams);
	assert(initResult == VRAPI_INITIALIZE_SUCCESS);
	
	vr_engine.java = java;

	return &vr_engine;
}

void VR_InitCvars( void )
{
	Cvar_Get ("skip_ioq3_credits", "0.0", CVAR_ARCHIVE);
	vr_worldscale = Cvar_Get ("vr_worldscale", "32.0", CVAR_ARCHIVE);
	vr_worldscaleScaler = Cvar_Get ("vr_worldscaleScaler", "1.0", CVAR_ARCHIVE);
	vr_hudDepth = Cvar_Get ("vr_hudDepth", "3", CVAR_ARCHIVE);
	vr_righthanded = Cvar_Get ("vr_righthanded", "1", CVAR_ARCHIVE);
	vr_switchThumbsticks = Cvar_Get ("vr_switchThumbsticks", "0", CVAR_ARCHIVE);
	vr_snapturn = Cvar_Get ("vr_snapturn", "45", CVAR_ARCHIVE);
	vr_extralatencymode = Cvar_Get ("vr_extralatencymode", "1", CVAR_ARCHIVE);
	vr_directionMode = Cvar_Get ("vr_directionMode", "0", CVAR_ARCHIVE); // 0 = HMD, 1 = Off-hand
	vr_weaponPitch = Cvar_Get ("vr_weaponPitch", "-20", CVAR_ARCHIVE);
	vr_heightAdjust = Cvar_Get ("vr_heightAdjust", "0.0", CVAR_ARCHIVE);
    vr_twoHandedWeapons = Cvar_Get ("vr_twoHandedWeapons", "1", CVAR_ARCHIVE);
    vr_showItemInHand = Cvar_Get ("vr_showItemInHand", "1", CVAR_ARCHIVE);
	vr_refreshrate = Cvar_Get ("vr_refreshrate", "72", CVAR_ARCHIVE);
    vr_weaponScope = Cvar_Get ("vr_weaponScope", "1", CVAR_ARCHIVE);
	vr_rollWhenHit = Cvar_Get ("vr_rollWhenHit", "0", CVAR_ARCHIVE);
	vr_hudYOffset = Cvar_Get ("vr_hudYOffset", "0", CVAR_ARCHIVE);
	vr_sendRollToServer = Cvar_Get ("vr_sendRollToServer", "1", CVAR_ARCHIVE);
	vr_lasersight = Cvar_Get ("vr_lasersight", "0", CVAR_ARCHIVE);
    vr_hapticIntensity = Cvar_Get ("vr_hapticIntensity", "1.0", CVAR_ARCHIVE);
    vr_comfortVignette = Cvar_Get ("vr_comfortVignette", "0.0", CVAR_ARCHIVE);
	vr_weaponSelectorMode = Cvar_Get ("vr_weaponSelectorMode", "0", CVAR_ARCHIVE);
	vr_weaponSelectorWithHud = Cvar_Get ("vr_weaponSelectorWithHud", "0", CVAR_ARCHIVE);
	vr_goreLevel = Cvar_Get ("vr_goreLevel", "2", CVAR_ARCHIVE);
	vr_hudDrawStatus = Cvar_Get ("vr_hudDrawStatus", "1", CVAR_ARCHIVE); // 0 - no hud, 1 - in-world hud, 2 - performance (static HUD)
	vr_showConsoleMessages = Cvar_Get ("vr_showConsoleMessages", "1", CVAR_ARCHIVE);

	// Values are:  scale,right,up,forward,pitch,yaw,roll
	// VALUES PROVIDED BY SkillFur - Thank-you!
	Cvar_Get ("vr_weapon_adjustment_1", "1,-4.0,7,-10,-20,-15,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_2", "0.8,-3.0,5.5,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_3", "0.8,-3.3,8,3.7,0,0,0", CVAR_ARCHIVE); // shotgun
	Cvar_Get ("vr_weapon_adjustment_4", "0.75,-5.4,6.5,-4,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_5", "0.8,-5.2,6,7.5,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_6", "0.8,-3.3,6,7,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_7", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_8", "0.8,-4.5,6,1.5,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_9", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);

	//Team Arena Weapons
	Cvar_Get ("vr_weapon_adjustment_11", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_12", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_13", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);

	// Control schemas
	Cvar_Get ("vr_uturn", "0", CVAR_ARCHIVE);
	Cvar_Get ("vr_controlSchema", "0", CVAR_ARCHIVE);
	qboolean uturnEnabled = Cvar_VariableValue( "vr_uturn" ) != 0;
	int controlSchema = (int)Cvar_VariableValue( "vr_controlSchema" ) % 3;
	if (controlSchema == 0) {
		// Default schema (weapon wheel on grip)
        Cvar_Get ("vr_button_map_RTHUMBLEFT", "turnleft", CVAR_ARCHIVE); // turn left
        Cvar_Get ("vr_button_map_RTHUMBRIGHT", "turnright", CVAR_ARCHIVE); // turn right
        Cvar_Get ("vr_button_map_RTHUMBFORWARD", "", CVAR_ARCHIVE); // unmapped
        if (uturnEnabled) {
            Cvar_Get ("vr_button_map_RTHUMBBACK", "uturn", CVAR_ARCHIVE); // u-turn
        } else {
            Cvar_Get ("vr_button_map_RTHUMBBACK", "", CVAR_ARCHIVE); // unmapped
        }
        Cvar_Get ("vr_button_map_PRIMARYGRIP", "+weapon_select", CVAR_ARCHIVE); // weapon selector
        Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARD_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBRIGHT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBLEFT_ALT", "", CVAR_ARCHIVE); // unmapped
    } else if (controlSchema == 1) {
	    // Weapon wheel on thumbstick - all directions as weapon select (useful for HMD wheel)
		Cvar_Get ("vr_button_map_RTHUMBFORWARD", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBRIGHT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBBACK", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBLEFT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_PRIMARYGRIP", "+alt", CVAR_ARCHIVE); // switch to alt layout
        Cvar_Get ("vr_button_map_RTHUMBLEFT_ALT", "turnleft", CVAR_ARCHIVE); // turn left
        Cvar_Get ("vr_button_map_RTHUMBRIGHT_ALT", "turnright", CVAR_ARCHIVE); // turn right
        Cvar_Get ("vr_button_map_RTHUMBFORWARD_ALT", "blank", CVAR_ARCHIVE);
        if (uturnEnabled) {
            Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "uturn", CVAR_ARCHIVE);
        } else {
            Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "blank", CVAR_ARCHIVE);
        }
	} else {
		// Weapon wheel disabled - only prev/next weapon switch is active
		Cvar_Get ("vr_button_map_RTHUMBLEFT", "turnleft", CVAR_ARCHIVE); // turn left
		Cvar_Get ("vr_button_map_RTHUMBRIGHT", "turnright", CVAR_ARCHIVE); // turn right
		Cvar_Get ("vr_button_map_RTHUMBFORWARD", "weapnext", CVAR_ARCHIVE); // next weapon
		if (uturnEnabled) {
			Cvar_Get ("vr_button_map_RTHUMBBACK", "uturn", CVAR_ARCHIVE); // u-turn
		} else {
			Cvar_Get ("vr_button_map_RTHUMBBACK", "weapprev", CVAR_ARCHIVE); // previous weapon
		}
		Cvar_Get ("vr_button_map_PRIMARYGRIP", "+alt", CVAR_ARCHIVE); // switch to alt layout
		Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK", "", CVAR_ARCHIVE); // unmapped
		Cvar_Get ("vr_button_map_RTHUMBFORWARD_ALT", "", CVAR_ARCHIVE); // unmapped
		Cvar_Get ("vr_button_map_RTHUMBRIGHT_ALT", "", CVAR_ARCHIVE); // unmapped
		Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "", CVAR_ARCHIVE); // unmapped
		Cvar_Get ("vr_button_map_RTHUMBLEFT_ALT", "", CVAR_ARCHIVE); // unmapped
	}

	// Map face buttons based on thumbstick assigned to movement
	// (cannot move and jump/crouch with same thumb at same time)
	qboolean switchThumbsticks = Cvar_VariableValue( "vr_switchThumbsticks" ) != 0;
	if (switchThumbsticks) {
		Cvar_Get ("vr_button_map_A", "+button2", CVAR_ARCHIVE); // Use Item
		Cvar_Get ("vr_button_map_B", "+button3", CVAR_ARCHIVE); // Gesture
		Cvar_Get ("vr_button_map_X", "+moveup", CVAR_ARCHIVE); // Jump
		Cvar_Get ("vr_button_map_Y", "+movedown", CVAR_ARCHIVE); // Crouch
	} else {
		Cvar_Get ("vr_button_map_A", "+moveup", CVAR_ARCHIVE); // Jump
		Cvar_Get ("vr_button_map_B", "+movedown", CVAR_ARCHIVE); // Crouch
		Cvar_Get ("vr_button_map_X", "+button2", CVAR_ARCHIVE); // Use Item
		Cvar_Get ("vr_button_map_Y", "+button3", CVAR_ARCHIVE); // Gesture
	}

	//Remaining button mapping (buttons not affected by schemas)
	Cvar_Get ("vr_button_map_A_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_B_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_X_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_Y_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_SECONDARYTHUMBSTICK", "+scores", CVAR_ARCHIVE); // Scoreboard
	Cvar_Get ("vr_button_map_SECONDARYTHUMBSTICK_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_SECONDARYTRIGGER", "+moveup", CVAR_ARCHIVE); // Also Jump
	Cvar_Get ("vr_button_map_SECONDARYTRIGGER_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_PRIMARYTRIGGER", "+attack", CVAR_ARCHIVE); // Fire
	Cvar_Get ("vr_button_map_PRIMARYTRIGGER_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_SECONDARYGRIP", "+weapon_stabilise", CVAR_ARCHIVE); // Weapon stabilisation
	Cvar_Get ("vr_button_map_SECONDARYGRIP_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_PRIMARYGRIP_ALT", "", CVAR_ARCHIVE); // unmapped
}

void VR_Destroy( engine_t* engine )
{
	if (engine == &vr_engine) {
		vrapi_Shutdown();
	}
}

void VR_EnterVR( engine_t* engine, ovrJava java ) {
	if (!engine->ovr) {
		ovrModeParms modeParams = vrapi_DefaultModeParms(&java);
		modeParams.Display = (size_t)eglGetCurrentDisplay();
		modeParams.WindowSurface = (size_t)eglGetCurrentSurface(EGL_DRAW);
		modeParams.ShareContext = (size_t)eglGetCurrentContext();

		engine->ovr = vrapi_EnterVrMode(&modeParams);
		engine->frameIndex = 0;

		vrapi_SetTrackingSpace(engine->ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);

        vrapi_SetClockLevels(engine->ovr, 4, 4);
	}
}

void VR_LeaveVR( engine_t* engine ) {
	if (engine->ovr) {
		vrapi_LeaveVrMode(engine->ovr);
		engine->ovr = NULL;
	}
}

engine_t* VR_GetEngine( void ) {
	return &vr_engine;
}

bool VR_isPauseable( void )
{
	return (bool)( ( clc.state == CA_ACTIVE) && !Cvar_VariableValue ("cl_paused") );
}

bool VR_useScreenLayer( void )
{
	//intermission is never full screen
    if ( cl.snap.ps.pm_type == PM_INTERMISSION )
    {
        return qfalse;
    }

    int keyCatcher = Key_GetCatcher( );
	return (bool)( clc.state == CA_CINEMATIC ||
			( keyCatcher & (KEYCATCH_UI | KEYCATCH_CONSOLE) ));
}
//#endif
