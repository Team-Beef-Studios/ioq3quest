#include "vr_base.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

//#if __ANDROID__

#include <assert.h>

static engine_t vr_engine;

const char* const requiredExtensionNames[] = {
		XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
		XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
		XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
		XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME,
		XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME,
		XR_KHR_COMPOSITION_LAYER_EQUIRECT2_EXTENSION_NAME,
		XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
		XR_FB_COLOR_SPACE_EXTENSION_NAME,
		XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME,
		XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME,
		XR_FB_FOVEATION_EXTENSION_NAME,
		XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME};
const uint32_t numRequiredExtensions =
		sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);

cvar_t *vr_worldscale = NULL;
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

engine_t* VR_Init( ovrJava java )
{
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(
            XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != NULL) {
        XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
        memset(&loaderInitializeInfoAndroid, 0, sizeof(loaderInitializeInfoAndroid));
        loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitializeInfoAndroid.next = NULL;
        loaderInitializeInfoAndroid.applicationVM = java.Vm;
        loaderInitializeInfoAndroid.applicationContext = java.ActivityObject;
        xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitializeInfoAndroid);
    }

    // Create the OpenXR instance.
    XrApplicationInfo appInfo;
    memset(&appInfo, 0, sizeof(appInfo));
    strcpy(appInfo.applicationName, "Quake 3 Arena");
    appInfo.applicationVersion = 0;
    strcpy(appInfo.engineName, "Quake 3 Arena");
    appInfo.engineVersion = 0;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo instanceCreateInfo;
    memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.next = NULL;
    instanceCreateInfo.createFlags = 0;
    instanceCreateInfo.applicationInfo = appInfo;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledApiLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = numRequiredExtensions;
    instanceCreateInfo.enabledExtensionNames = requiredExtensionNames;
    if (xrCreateInstance(&instanceCreateInfo, &vr_engine.Instance) != XR_SUCCESS) {
        exit(1);
    }

    memset(&vr_engine, 0, sizeof(vr_engine));
    vr_engine.java = java;
    return &vr_engine;
}

void VR_InitCvars( void )
{
	Cvar_Get ("skip_ioq3_credits", "0.0", CVAR_ARCHIVE);
	vr_worldscale = Cvar_Get ("vr_worldscale", "32.0", CVAR_ARCHIVE);
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
	Cvar_Get ("vr_weapon_adjustment_10", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_11", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);
	Cvar_Get ("vr_weapon_adjustment_12", "0.8,-5.5,6,0,0,0,0", CVAR_ARCHIVE);

	// Control schemas
	Cvar_Get ("vr_uturn", "0", CVAR_ARCHIVE);
	Cvar_Get ("vr_controlSchema", "0", CVAR_ARCHIVE);
	qboolean uturnEnabled = Cvar_VariableValue( "vr_uturn" ) != 0;
	int controlSchema = (int)Cvar_VariableValue( "vr_controlSchema" ) % 2;
	if (controlSchema == 0) {
        Cvar_Get ("vr_button_map_RTHUMBLEFT", "turnleft", CVAR_ARCHIVE); // turn left
        Cvar_Get ("vr_button_map_RTHUMBRIGHT", "turnright", CVAR_ARCHIVE); // turn right
        Cvar_Get ("vr_button_map_RTHUMBFORWARD", "weapnext", CVAR_ARCHIVE); // next weapon
        if (uturnEnabled) {
            Cvar_Get ("vr_button_map_RTHUMBBACK", "uturn", CVAR_ARCHIVE); // u-turn
        } else {
            Cvar_Get ("vr_button_map_RTHUMBBACK", "weapprev", CVAR_ARCHIVE); // previous weapon
        }
        Cvar_Get ("vr_button_map_PRIMARYGRIP", "+weapon_select", CVAR_ARCHIVE); // weapon selector
        Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARD_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARDRIGHT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARDRIGHT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBRIGHT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACKRIGHT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACKRIGHT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACKLEFT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBBACKLEFT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBLEFT_ALT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARDLEFT", "", CVAR_ARCHIVE); // unmapped
       	Cvar_Get ("vr_button_map_RTHUMBFORWARDLEFT_ALT", "", CVAR_ARCHIVE); // unmapped
	} else {
	    // All directions as weapon select (useful for HMD wheel)
		Cvar_Get ("vr_button_map_RTHUMBFORWARD", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBFORWARDRIGHT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBRIGHT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBBACKRIGHT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBBACK", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBBACKLEFT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBLEFT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_RTHUMBFORWARDLEFT", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_PRIMARYTHUMBSTICK", "+weapon_select", CVAR_ARCHIVE);
    	Cvar_Get ("vr_button_map_PRIMARYGRIP", "+alt", CVAR_ARCHIVE); // switch to alt layout
        Cvar_Get ("vr_button_map_RTHUMBLEFT_ALT", "turnleft", CVAR_ARCHIVE); // turn left
        Cvar_Get ("vr_button_map_RTHUMBRIGHT_ALT", "turnright", CVAR_ARCHIVE); // turn right
        Cvar_Get ("vr_button_map_RTHUMBFORWARD_ALT", "weapnext", CVAR_ARCHIVE);
        if (uturnEnabled) {
            Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "uturn", CVAR_ARCHIVE);
        } else {
            Cvar_Get ("vr_button_map_RTHUMBBACK_ALT", "weapprev", CVAR_ARCHIVE);
        }
        Cvar_Get ("vr_button_map_RTHUMBFORWARDRIGHT_ALT", "blank", CVAR_ARCHIVE); // unmapped
        Cvar_Get ("vr_button_map_RTHUMBBACKRIGHT_ALT", "blank", CVAR_ARCHIVE); // unmapped
        Cvar_Get ("vr_button_map_RTHUMBBACKLEFT_ALT", "blank", CVAR_ARCHIVE); // unmapped
        Cvar_Get ("vr_button_map_RTHUMBFORWARDLEFT_ALT", "blank", CVAR_ARCHIVE); // unmapped
	}

	//Remaining button mapping (buttons not affected by schemas)
	Cvar_Get ("vr_button_map_A", "+moveup", CVAR_ARCHIVE); // Jump
	Cvar_Get ("vr_button_map_A_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_B", "+movedown", CVAR_ARCHIVE); // Crouch
	Cvar_Get ("vr_button_map_B_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_X", "+button2", CVAR_ARCHIVE); // Use Item
	Cvar_Get ("vr_button_map_X_ALT", "", CVAR_ARCHIVE); // unmapped
	Cvar_Get ("vr_button_map_Y", "+button3", CVAR_ARCHIVE); // Gesture
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
        xrDestroyInstance(engine->Instance);
    }
}

void VR_EnterVR( engine_t* engine, ovrJava java ) {

    XrSystemId systemId;
    XrSystemGetInfo systemGetInfo;
    memset(&systemGetInfo, 0, sizeof(systemGetInfo));
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.next = NULL;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (xrGetSystem(engine->Instance, &systemGetInfo, &systemId) != XR_SUCCESS) {
        exit(1);
    }

    // Get the graphics requirements.
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
    xrGetInstanceProcAddr(
            engine->Instance,
            "xrGetOpenGLESGraphicsRequirementsKHR",
            (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR));
    XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    pfnGetOpenGLESGraphicsRequirementsKHR(engine->Instance, systemId, &graphicsRequirements);

    // Create the OpenXR Session.
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingAndroidGLES = {};
    graphicsBindingAndroidGLES.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    graphicsBindingAndroidGLES.next = NULL;
    graphicsBindingAndroidGLES.display = eglGetCurrentDisplay();
    graphicsBindingAndroidGLES.config = eglGetCurrentSurface(EGL_DRAW);
    graphicsBindingAndroidGLES.context = eglGetCurrentContext();

    XrSessionCreateInfo sessionCreateInfo = {};
    memset(&sessionCreateInfo, 0, sizeof(sessionCreateInfo));
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBindingAndroidGLES;
    sessionCreateInfo.createFlags = 0;
    sessionCreateInfo.systemId = systemId;

    if (xrCreateSession(engine->Instance, &sessionCreateInfo, &engine->Session) != XR_SUCCESS) {
        exit(1);
    }
}

void VR_LeaveVR( engine_t* engine ) {
    xrDestroySession(engine->Session);
}

engine_t* VR_GetEngine( void ) {
	return &vr_engine;
}

int VR_useScreenLayer( void )
{
	//intermission is never full screen
    if ( cl.snap.ps.pm_type == PM_INTERMISSION )
    {
        return 0;
    }

    int keyCatcher = Key_GetCatcher( );
	return ( clc.state == CA_CINEMATIC ||
			( keyCatcher & (KEYCATCH_UI | KEYCATCH_CONSOLE) ));
}
//#endif
