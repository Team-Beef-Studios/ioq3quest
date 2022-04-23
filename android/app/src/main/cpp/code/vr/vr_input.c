#include "vr_input.h"

//#if __ANDROID__

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/keycodes.h"
#include "../client/client.h"
#include "vr_base.h"
#include "../VrApi/Include/VrApi_Input.h"
#include "../VrApi/Include/VrApi_Helpers.h"
#include "vr_clientinfo.h"

#include <unistd.h>
#include <jni.h>

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

enum {
	VR_TOUCH_AXIS_UP = 1 << 0,
	VR_TOUCH_AXIS_UPRIGHT = 1 << 1,
	VR_TOUCH_AXIS_RIGHT = 1 << 2,
	VR_TOUCH_AXIS_DOWNRIGHT = 1 << 3,
	VR_TOUCH_AXIS_DOWN = 1 << 4,
	VR_TOUCH_AXIS_DOWNLEFT = 1 << 5,
	VR_TOUCH_AXIS_LEFT = 1 << 6,
	VR_TOUCH_AXIS_UPLEFT = 1 << 7,
	VR_TOUCH_AXIS_TRIGGER_INDEX = 1 << 8,
};

typedef struct {
	uint32_t buttons;
	uint32_t axisButtons;
} vrController_t;

vr_clientinfo_t vr;

static qboolean controllerInit = qfalse;

static vrController_t leftController;
static vrController_t rightController;
static int in_vrEventTime = 0;
static double lastframetime = 0;

static float triggerPressedThreshold = 0.75f;
static float triggerReleasedThreshold = 0.5f;

static float thumbstickPressedThreshold = 0.5f;
static float thumbstickReleasedThreshold = 0.4f;

extern cvar_t *cl_sensitivity;
extern cvar_t *m_pitch;
extern cvar_t *m_yaw;

float radians(float deg) {
    return (deg * M_PI) / 180.0;
}

float degrees(float rad) {
    return (rad * 180.0) / M_PI;
}


#ifndef EPSILON
#define EPSILON 0.001f
#endif

extern cvar_t *vr_righthanded;
extern cvar_t *vr_switchThumbsticks;
extern cvar_t *vr_snapturn;
extern cvar_t *vr_extralatencymode;
extern cvar_t *vr_directionMode;
extern cvar_t *vr_weaponPitch;
extern cvar_t *vr_heightAdjust;
extern cvar_t *vr_twoHandedWeapons;
extern cvar_t *vr_refreshrate;
extern cvar_t *vr_weaponScope;
extern cvar_t *vr_hapticIntensity;

jclass callbackClass;
jmethodID android_haptic_event;

qboolean alt_key_mode_active = qfalse;

void rotateAboutOrigin(float x, float y, float rotation, vec2_t out)
{
	out[0] = cosf(DEG2RAD(-rotation)) * x  +  sinf(DEG2RAD(-rotation)) * y;
	out[1] = cosf(DEG2RAD(-rotation)) * y  -  sinf(DEG2RAD(-rotation)) * x;
}

static ovrVector3f normalizeVec(ovrVector3f vec) {
    //NOTE: leave w-component untouched
    //@@const float EPSILON = 0.000001f;
    float xxyyzz = vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
    //@@if(xxyyzz < EPSILON)
    //@@    return *this; // do nothing if it is zero vector

    //float invLength = invSqrt(xxyyzz);
    ovrVector3f result;
    float invLength = 1.0f / sqrtf(xxyyzz);
    result.x = vec.x * invLength;
    result.y = vec.y * invLength;
    result.z = vec.z * invLength;
    return result;
}

static float length(float x, float y)
{
    return sqrtf(powf(x, 2.0f) + powf(y, 2.0f));
}

void NormalizeAngles(vec3_t angles)
{
    while (angles[0] >= 90) angles[0] -= 180;
    while (angles[1] >= 180) angles[1] -= 360;
    while (angles[2] >= 180) angles[2] -= 360;
    while (angles[0] < -90) angles[0] += 180;
    while (angles[1] < -180) angles[1] += 360;
    while (angles[2] < -180) angles[2] += 360;
}

void GetAnglesFromVectors(const ovrVector3f forward, const ovrVector3f right, const ovrVector3f up, vec3_t angles)
{
    float sr, sp, sy, cr, cp, cy;

    sp = -forward.z;

    float cp_x_cy = forward.x;
    float cp_x_sy = forward.y;
    float cp_x_sr = -right.z;
    float cp_x_cr = up.z;

    float yaw = atan2(cp_x_sy, cp_x_cy);
    float roll = atan2(cp_x_sr, cp_x_cr);

    cy = cos(yaw);
    sy = sin(yaw);
    cr = cos(roll);
    sr = sin(roll);

    if (fabs(cy) > EPSILON)
    {
        cp = cp_x_cy / cy;
    }
    else if (fabs(sy) > EPSILON)
    {
        cp = cp_x_sy / sy;
    }
    else if (fabs(sr) > EPSILON)
    {
        cp = cp_x_sr / sr;
    }
    else if (fabs(cr) > EPSILON)
    {
        cp = cp_x_cr / cr;
    }
    else
    {
        cp = cos(asin(sp));
    }

    float pitch = atan2(sp, cp);

    angles[0] = pitch / (M_PI*2.f / 360.f);
    angles[1] = yaw / (M_PI*2.f / 360.f);
    angles[2] = roll / (M_PI*2.f / 360.f);

    NormalizeAngles(angles);
}

void QuatToYawPitchRoll(ovrQuatf q, vec3_t rotation, vec3_t out) {

    ovrMatrix4f mat = ovrMatrix4f_CreateFromQuaternion( &q );

    if (rotation[0] != 0.0f || rotation[1] != 0.0f || rotation[2] != 0.0f)
    {
        ovrMatrix4f rot = ovrMatrix4f_CreateRotation(radians(rotation[0]), radians(rotation[1]), radians(rotation[2]));
        mat = ovrMatrix4f_Multiply(&mat, &rot);
    }

    ovrVector4f v1 = {0, 0, -1, 0};
    ovrVector4f v2 = {1, 0, 0, 0};
    ovrVector4f v3 = {0, 1, 0, 0};

    ovrVector4f forwardInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v1);
    ovrVector4f rightInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v2);
    ovrVector4f upInVRSpace = ovrVector4f_MultiplyMatrix4f(&mat, &v3);

    ovrVector3f forward = {-forwardInVRSpace.z, -forwardInVRSpace.x, forwardInVRSpace.y};
    ovrVector3f right = {-rightInVRSpace.z, -rightInVRSpace.x, rightInVRSpace.y};
    ovrVector3f up = {-upInVRSpace.z, -upInVRSpace.x, upInVRSpace.y};

    ovrVector3f forwardNormal = normalizeVec(forward);
    ovrVector3f rightNormal = normalizeVec(right);
    ovrVector3f upNormal = normalizeVec(up);

    GetAnglesFromVectors(forwardNormal, rightNormal, upNormal, out);
}

//0 = left, 1 = right
float vibration_channel_duration[2] = {0.0f, 0.0f};
float vibration_channel_intensity[2] = {0.0f, 0.0f};
ovrDeviceID controllerIDs[2];

void VR_Vibrate( int duration, int chan, float intensity )
{
    for (int i = 0; i < 2; ++i)
    {
        int channel = (i + 1) & chan;
        if (channel)
        {
            if (vibration_channel_duration[channel-1] > 0.0f)
                return;

            if (vibration_channel_duration[channel-1] == -1.0f && duration != 0.0f)
                return;

            vibration_channel_duration[channel-1] = duration;
            vibration_channel_intensity[channel-1] = intensity * vr_hapticIntensity->value;
        }
    }
}


static void VR_processHaptics() {
    static float lastFrameTime = 0.0f;
    float timestamp = (float)(Sys_Milliseconds( ));
    float frametime = timestamp - lastFrameTime;
    lastFrameTime = timestamp;

    for (int i = 0; i < 2; ++i) {
        if (vibration_channel_duration[i] > 0.0f ||
            vibration_channel_duration[i] == -1.0f) {
            vrapi_SetHapticVibrationSimple(VR_GetEngine()->ovr, controllerIDs[i],
                                           vibration_channel_intensity[i]);

            if (vibration_channel_duration[i] != -1.0f) {
                vibration_channel_duration[i] -= frametime;

                if (vibration_channel_duration[i] < 0.0f) {
                    vibration_channel_duration[i] = 0.0f;
                    vibration_channel_intensity[i] = 0.0f;
                }
            }
        } else {
            vrapi_SetHapticVibrationSimple(VR_GetEngine()->ovr, controllerIDs[i], 0.0f);
        }
    }
}

static qboolean IN_GetInputAction(const char* inputName, char* action)
{
    char cvarname[256];
    Com_sprintf(cvarname, 256, "vr_button_map_%s%s", inputName, alt_key_mode_active ? "_ALT" : "");
    char * val = Cvar_VariableString(cvarname);
    if (val && strlen(val) > 0)
    {
        Com_sprintf(action, 256, "%s", val);
        return qtrue;
    }

    //If we didn't find something for this input and the alt key is active, then see if the un-alt key has a function
    if (alt_key_mode_active)
    {
        Com_sprintf(cvarname, 256, "vr_button_map_%s", inputName);
        char * val = Cvar_VariableString(cvarname);
        if (val && strlen(val) > 0)
        {
            Com_sprintf(action, 256, "%s", val);
            return qtrue;
        }
    }

    return qfalse;
}

// Returns true in case active input should be auto-repeated (now only applicable for smooth-turn)
static qboolean IN_SendInputAction(const char* action, qboolean inputActive, float axisValue, qboolean thumbstickAxis)
{
    if (action)
    {
        //handle our special actions first
        if (strcmp(action, "blank") == 0) {
            // Empty function used to block alt fallback on unmapped alt buttons or
            // force 8-way mapping mode of thumbstick without assigning actual action
        }
        else if (strcmp(action, "+alt") == 0)
        {
            alt_key_mode_active = inputActive;
        }
        else if (strcmp(action, "+weapon_stabilise") == 0)
        {
            //stabilised weapon only triggered when controllers close enough (40cm) to each other
            if (inputActive)
            {
                vec3_t l;
                VectorSubtract(vr.weaponposition, vr.offhandposition, l);
                vr.weapon_stabilised =  VectorLength(l) < 0.4f;
            }
            else
            {
                vr.weapon_stabilised =  qfalse;
            }
        }
        //Special case for moveup as we can send a space key instead allowing us to skip
        //server search in the server menu
        else if (strcmp(action, "+moveup") == 0)
        {
            Com_QueueEvent(in_vrEventTime, SE_KEY, K_SPACE, inputActive, 0, NULL);
        }
        else if (strcmp(action, "+weapon_select") == 0)
        {
            vr.weapon_select = inputActive;
            if (inputActive) {
                int selectorType = (int) Cvar_VariableValue("vr_weaponSelectorMode");
                vr.weapon_select_using_thumbstick = (selectorType == WS_HMD);
                vr.weapon_select_autoclose = vr.weapon_select_using_thumbstick && thumbstickAxis;
            } else {
                vr.weapon_select_using_thumbstick = qfalse;
                vr.weapon_select_autoclose = qfalse;
                Cbuf_AddText("weapon_select");
            }
        }
        else if (action[0] == '+')
        {
            char command[256];
            Com_sprintf(command, sizeof(command), "%s%s\n", inputActive ? "+" : "-", action + 1);
            Cbuf_AddText(command);
        }
        else if (inputActive)
        {
            if (strcmp(action, "turnleft") == 0) {
                if (vr_snapturn->integer > 0) { // snap turn
                    int snap = 45;
                    if (vr_snapturn->integer > 1) {
                        snap = vr_snapturn->integer;
                    }
                    CL_SnapTurn(-snap);
                } else { // yaw (smooth turn)
                    float value = (axisValue > 0.0f ? axisValue : 1.0f) * cl_sensitivity->value * m_yaw->value;
                    Com_QueueEvent(in_vrEventTime, SE_MOUSE, -value, 0, 0, NULL);
                    return qtrue;
                }
            } else if (strcmp(action, "turnright") == 0) {
                if (vr_snapturn->integer > 0) { // snap turn
                    int snap = 45;
                    if (vr_snapturn->integer > 1) {
                        snap = vr_snapturn->integer;
                    }
                    CL_SnapTurn(snap);
                } else { // yaw (smooth turn)
                    float value = (axisValue > 0.0f ? axisValue : 1.0f) * cl_sensitivity->value * m_yaw->value;
                    Com_QueueEvent(in_vrEventTime, SE_MOUSE, value, 0, 0, NULL);
                    return qtrue;
                }
            } else if (strcmp(action, "uturn") == 0) {
                CL_SnapTurn(180);
            } else {
                char command[256];
                Com_sprintf(command, sizeof(command), "%s\n", action);
                Cbuf_AddText(command);
            }
        }
    }
    return qfalse;
}

static void IN_ActivateInput(uint32_t * inputGroup, int inputFlag) {
    *inputGroup |= inputFlag;
}

static void IN_DeactivateInput(uint32_t * inputGroup, int inputFlag) {
    *inputGroup &= ~inputFlag;
}

static qboolean IN_InputActivated(uint32_t * inputGroup, int inputFlag) {
    return (*inputGroup & inputFlag);
}

static void IN_HandleActiveInput(uint32_t * inputGroup, int inputFlag, char* inputName, float axisValue, qboolean thumbstickAxis) {
    if (IN_InputActivated(inputGroup, inputFlag)) {
        // Input is already in activated state, nothing to do
        return;
    }
    char action[256];
    if (IN_GetInputAction(inputName, action)) {
        // Activate input action
        if (!IN_SendInputAction(action, qtrue, axisValue, thumbstickAxis)) {
            // Action should not be repeated, mark input as activated
            IN_ActivateInput(inputGroup, inputFlag);
        }
    } else {
        // No assigned action -> mark input as activated
        // (to avoid unnecessary action lookup next time)
        IN_ActivateInput(inputGroup, inputFlag);
    }
}

static void IN_HandleInactiveInput(uint32_t * inputGroup, int inputFlag, char* inputName, float axisValue, qboolean thumbstickAxis) {
    if (!IN_InputActivated(inputGroup, inputFlag)) {
        // Input is not in activated state, nothing to do
        return;
    }
    char action[256];
    if (IN_GetInputAction(inputName, action)) {
        // Deactivate input action and remove input activated state
        IN_SendInputAction(action, qfalse, axisValue, thumbstickAxis);
        IN_DeactivateInput(inputGroup, inputFlag);
    } else {
        // No assigned action -> just remove input activated state
        IN_DeactivateInput(inputGroup, inputFlag);
    }
}

void VR_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
    if (vr_hapticIntensity->value == 0.0f)
    {
        return;
    }

    engine_t* engine = VR_GetEngine();
    jstring StringArg1 = (*(engine->java.Env))->NewStringUTF(engine->java.Env, event);
    (*(engine->java.Env))->CallVoidMethod(engine->java.Env, engine->java.ActivityObject, android_haptic_event, StringArg1, position, flags, (int)(intensity * vr_hapticIntensity->value), angle, yHeight);

    //Controller Haptic Support
    int weaponFireChannel = vr.weapon_stabilised ? 3 : (vr_righthanded->integer ? 2 : 1);
    if (strcmp(event, "pickup_shield") == 0 ||
            strcmp(event, "pickup_weapon") == 0 ||
            strstr(event, "pickup_item") != NULL)
    {
        VR_Vibrate(100, 3, 1.0);
    }
    else if (strcmp(event, "weapon_switch") == 0)
    {
        VR_Vibrate(250, vr_righthanded->integer ? 2 : 1, 0.8);
    }
    else if (strcmp(event, "shotgun") == 0 || strcmp(event, "fireball") == 0)
    {
        VR_Vibrate(400, 3, 1.0);
    }
    else if (strcmp(event, "bullet") == 0)
    {
        VR_Vibrate(150, 3, 1.0);
    }
    else if (strcmp(event, "chainsaw_fire") == 0 ||
        strcmp(event, "RTCWQuest:fire_tesla") == 0)
    {
        VR_Vibrate(500, weaponFireChannel, 1.0);
    }
    else if (strcmp(event, "machinegun_fire") == 0 || strcmp(event, "plasmagun_fire") == 0)
    {
        VR_Vibrate(90, weaponFireChannel, 0.8);
    }
    else if (strcmp(event, "shotgun_fire") == 0)
    {
        VR_Vibrate(250, weaponFireChannel, 1.0);
    }
    else if (strcmp(event, "rocket_fire") == 0 ||
        strcmp(event, "RTCWQuest:fire_sniper") == 0 ||
        strcmp(event, "bfg_fire") == 0 ||
        strcmp(event, "handgrenade_fire") == 0 )
    {
        VR_Vibrate(400, weaponFireChannel, 1.0);
    }
    else if (strcmp(event, "selector_icon") == 0)
    {
        //Quick blip
        VR_Vibrate(50, (vr_righthanded->integer ? 2 : 1), 1.0);
    }
}

void IN_VRInit( void )
{
	memset(&vr, 0, sizeof(vr));

	engine_t *engine = VR_GetEngine();
    callbackClass = (*engine->java.Env)->GetObjectClass(engine->java.Env, engine->java.ActivityObject);
    android_haptic_event = (*engine->java.Env)->GetMethodID(engine->java.Env, callbackClass, "haptic_event","(Ljava/lang/String;IIIFF)V");
}

static void IN_VRController( qboolean isRightController, ovrTracking remoteTracking )
{
    if (isRightController == (vr_righthanded->integer != 0))
	{
		//Set gun angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
		vec3_t rotation = {0};
		rotation[PITCH] = vr_weaponPitch->value;
		QuatToYawPitchRoll(remoteTracking.HeadPose.Pose.Orientation, rotation, vr.weaponangles);

		VectorSubtract(vr.weaponangles_last, vr.weaponangles, vr.weaponangles_delta);
		VectorCopy(vr.weaponangles, vr.weaponangles_last);

		///Weapon location relative to view
		vr.weaponposition[0] = remoteTracking.HeadPose.Pose.Position.x;
		vr.weaponposition[1] = remoteTracking.HeadPose.Pose.Position.y + vr_heightAdjust->value;
		vr.weaponposition[2] = remoteTracking.HeadPose.Pose.Position.z;

		VectorCopy(vr.weaponoffset_last[1], vr.weaponoffset_last[0]);
		VectorCopy(vr.weaponoffset, vr.weaponoffset_last[1]);
		VectorSubtract(vr.weaponposition, vr.hmdposition, vr.weaponoffset);
	} else {
        vec3_t rotation = {0};
        QuatToYawPitchRoll(remoteTracking.HeadPose.Pose.Orientation, rotation, vr.offhandangles2); // used for off-hand direction mode
        rotation[PITCH] = vr_weaponPitch->value;
        QuatToYawPitchRoll(remoteTracking.HeadPose.Pose.Orientation, rotation, vr.offhandangles);

        ///location relative to view
        vr.offhandposition[0] = remoteTracking.HeadPose.Pose.Position.x;
        vr.offhandposition[1] = remoteTracking.HeadPose.Pose.Position.y + vr_heightAdjust->value;
        vr.offhandposition[2] = remoteTracking.HeadPose.Pose.Position.z;

        VectorCopy(vr.offhandoffset_last[1], vr.offhandoffset_last[0]);
        VectorCopy(vr.offhandoffset, vr.offhandoffset_last[1]);
        VectorSubtract(vr.offhandposition, vr.hmdposition, vr.offhandoffset);
	}

	if (vr.virtual_screen || cl.snap.ps.pm_type == PM_INTERMISSION)
    {
        vr.weapon_zoomed = qfalse;
        if (vr.menuCursorX && vr.menuCursorY)
        {
            float yaw;
            float pitch;
            if (vr.menuLeftHanded) {
                yaw = (vr_righthanded->integer != 0) ? vr.offhandangles[YAW] : vr.weaponangles[YAW];
                pitch = (vr_righthanded->integer != 0) ? vr.offhandangles[PITCH] : vr.weaponangles[PITCH];
            } else {
                yaw = (vr_righthanded->integer != 0) ? vr.weaponangles[YAW] : vr.offhandangles[YAW];
                pitch = (vr_righthanded->integer != 0) ? vr.weaponangles[PITCH] : vr.offhandangles[PITCH];
            }
            int x = 320 - tan((yaw - vr.menuYaw) * (M_PI*2 / 360)) * 400;
            int y = 240 + tan((pitch + vr_weaponPitch->value) * (M_PI*2 / 360)) * 400;
            *vr.menuCursorX = x;
            *vr.menuCursorY = y;
            Com_QueueEvent(in_vrEventTime, SE_MOUSE, 0, 0, 0, NULL);
        }
    }
    else
    {

        vr.weapon_zoomed = vr_weaponScope->integer &&
                           vr.weapon_stabilised &&
                           (cl.snap.ps.weapon == WP_RAILGUN) &&
                           (VectorLength(vr.weaponoffset) < 0.24f) &&
                           cl.snap.ps.stats[STAT_HEALTH] > 0;

        if (vr_twoHandedWeapons->integer && vr.weapon_stabilised)
        {
            //Apply smoothing to the weapon hand
            vec3_t smooth_weaponoffset;
            VectorAdd(vr.weaponoffset, vr.weaponoffset_last[0], smooth_weaponoffset);
            VectorAdd(smooth_weaponoffset, vr.weaponoffset_last[1],smooth_weaponoffset);
            VectorScale(smooth_weaponoffset, 1.0f/3.0f, smooth_weaponoffset);

            vec3_t vec;
            VectorSubtract(vr.offhandoffset, smooth_weaponoffset, vec);

            float zxDist = length(vec[0], vec[2]);

            if (zxDist != 0.0f && vec[2] != 0.0f) {
                VectorSet(vr.weaponangles, -degrees(atanf(vec[1] / zxDist)),
                          -degrees(atan2f(vec[0], -vec[2])), vr.weaponangles[ROLL] / 2.0f); //Dampen roll on stabilised weapon
            }
        }
    }
}

static qboolean IN_VRJoystickUse8WayMapping( void ) {
    char action[256];
    return IN_GetInputAction("RTHUMBFORWARDRIGHT", action)
        || IN_GetInputAction("RTHUMBBACKRIGHT", action)
        || IN_GetInputAction("RTHUMBBACKLEFT", action)
        || IN_GetInputAction("RTHUMBFORWARDLEFT", action);
}

static void IN_VRJoystickHandle4WayMapping( uint32_t * inputGroup, float joystickAngle, float joystickValue ) {
    if (joystickAngle >= 315.0 || joystickAngle < 45.0) { // UP
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        // Activate UP
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
    } else if (joystickAngle < 135.0) { // RIGHT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        // Activate RIGHT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
    } else if (joystickAngle < 225.0) { // DOWN
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        // Activate DOWN
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
    } else { // LEFT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        // Activate LEFT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
    }
}

static void IN_VRJoystickHandle8WayMapping( uint32_t * inputGroup, float joystickAngle, float joystickValue ) {
    if (joystickAngle > 337.5 || joystickAngle < 22.5) { // UP
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        // Activate UP
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
    } else if (joystickAngle < 67.5) { // UP-RIGHT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        // Activate UP-RIGHT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
    } else if (joystickAngle < 112.5) { // RIGHT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        // Activate RIGHT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
    } else if (joystickAngle < 157.5) { // DOWN-RIGHT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        // Activate DOWN-RIGHT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
    } else if (joystickAngle < 202.5) { // DOWN
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        // Activate DOWN
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
    } else if (joystickAngle < 247.5) { // DOWN-LEFT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        // Activate DOWN-LEFT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
    } else if (joystickAngle < 292.5) { // LEFT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        // Activate LEFT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
    } else { // UP-LEFT
        // Deactivate neighboring inputs
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
        IN_HandleInactiveInput(inputGroup, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
        // Activate UP-LEFT
        IN_HandleActiveInput(inputGroup, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
    }
}

static void IN_VRJoystick( qboolean isRightController, float joystickX, float joystickY )
{
	vrController_t* controller = isRightController == qtrue ? &rightController : &leftController;

    vr.thumbstick_location[isRightController][0] = joystickX;
    vr.thumbstick_location[isRightController][1] = joystickY;

	if (vr.virtual_screen || cl.snap.ps.pm_type == PM_INTERMISSION)
	{

	    // Use thumbstick UP/DOWN as PAGEUP/PAGEDOWN in menus
        if (joystickY > thumbstickPressedThreshold) {
            if (!IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_UP)) {
                IN_ActivateInput(&controller->axisButtons, VR_TOUCH_AXIS_UP);
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_PGUP, qtrue, 0, NULL);
            }
        } else if (joystickY < -thumbstickPressedThreshold) {
            if (!IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_DOWN)) {
                IN_ActivateInput(&controller->axisButtons, VR_TOUCH_AXIS_DOWN);
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_PGDN, qtrue, 0, NULL);
            }
        } else if (joystickY < thumbstickReleasedThreshold && joystickY > -thumbstickReleasedThreshold) {
            if (IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_UP)) {
                IN_DeactivateInput(&controller->axisButtons, VR_TOUCH_AXIS_UP);
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_PGUP, qfalse, 0, NULL);
            }
            if (IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_DOWN)) {
                IN_DeactivateInput(&controller->axisButtons, VR_TOUCH_AXIS_DOWN);
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_PGDN, qfalse, 0, NULL);
            }
        }

	}
	else
	{

		if (isRightController == (vr_switchThumbsticks->integer != 0)) {
			vec3_t positional;
			VectorClear(positional);

			vec2_t joystick;
            if ( vr.use_fake_6dof )
            {
                //multiplayer game
                if (!vr_directionMode->integer) {
					//HMD Based
					rotateAboutOrigin(joystickX, joystickY, vr.hmdorientation[YAW], joystick);
				} else {
                	//Off-hand based
					rotateAboutOrigin(joystickX, joystickY, vr.offhandangles2[YAW], joystick);
				}
            }
            else
            {
				//Positional movement speed correction for when we are not hitting target framerate
				int refresh = vrapi_GetSystemPropertyInt(&(VR_GetEngine()->java), VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE);
				float multiplier = (float)((1000.0 / refresh) / (in_vrEventTime - lastframetime));

				float factor = (refresh / 72.0F) * 10.0f; // adjust positional factor based on refresh rate
				rotateAboutOrigin(-vr.hmdposition_delta[0] * factor * multiplier,
								  vr.hmdposition_delta[2] * factor * multiplier, -vr.hmdorientation[YAW], positional);

				if (!vr_directionMode->integer) {
					//HMD Based
					joystick[0] = joystickX;
					joystick[1] = joystickY;
				} else {
					//Off-hand based
					rotateAboutOrigin(joystickX, joystickY, vr.offhandangles2[YAW] - vr.hmdorientation[YAW], joystick);
				}
            }

            //sideways
            Com_QueueEvent(in_vrEventTime, SE_JOYSTICK_AXIS, 0, joystick[0] * 127.0f + positional[0] * 127.0f, 0, NULL);

            //forward/back
            Com_QueueEvent(in_vrEventTime, SE_JOYSTICK_AXIS, 1, joystick[1] * 127.0f + positional[1] * 127.0f, 0, NULL);
        }

        // In case thumbstick is used by weapon wheel (is in HMD/thumbstick mode), ignore standard thumbstick inputs
        else if (!vr.weapon_select_using_thumbstick)
        {
            float joystickValue = length(joystickX, joystickY);
            if (joystickValue < thumbstickReleasedThreshold) {
                // Joystick within threshold -> disable all inputs
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_UP, "RTHUMBFORWARD", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_UPRIGHT, "RTHUMBFORWARDRIGHT", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_RIGHT, "RTHUMBRIGHT", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_DOWNRIGHT, "RTHUMBBACKRIGHT", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_DOWN, "RTHUMBBACK", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_DOWNLEFT, "RTHUMBBACKLEFT", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_LEFT, "RTHUMBLEFT", joystickValue, qtrue);
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_UPLEFT, "RTHUMBFORWARDLEFT", joystickValue, qtrue);
            } else if (joystickValue > thumbstickPressedThreshold) {
                float joystickAngle = AngleNormalize360(RAD2DEG(atan2(joystickX, joystickY)));
                if (IN_VRJoystickUse8WayMapping()) {
                    IN_VRJoystickHandle8WayMapping(&controller->axisButtons, joystickAngle, joystickValue);
                } else {
                    IN_VRJoystickHandle4WayMapping(&controller->axisButtons, joystickAngle, joystickValue);
                }
            }
        }

    }
}

static void IN_VRTriggers( qboolean isRightController, float triggerValue ) {
	vrController_t* controller = isRightController == qtrue ? &rightController : &leftController;

	if (VR_useScreenLayer() || cl.snap.ps.pm_type == PM_INTERMISSION) {

    	// Triggers are used for menu navigation in screen mode or in intermission
        if (triggerValue > triggerPressedThreshold && !IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX))
        {
            IN_ActivateInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX);
            if ((isRightController && !vr.menuLeftHanded) || (!isRightController && vr.menuLeftHanded)) {
                // Active controller confirms selection
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_MOUSE1, qtrue, 0, NULL);
                VR_Vibrate(200, vr.menuLeftHanded ? 1 : 2, 0.8);
            } else {
                // Inactive controller becomes active one
                vr.menuLeftHanded = !vr.menuLeftHanded;
            }
        }
        else if (triggerValue < triggerReleasedThreshold && IN_InputActivated(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX))
        {
            IN_DeactivateInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX);
            if ((isRightController && !vr.menuLeftHanded) || (!isRightController && vr.menuLeftHanded)) {
                Com_QueueEvent(in_vrEventTime, SE_KEY, K_MOUSE1, qfalse, 0, NULL);
            }
        }

    } else {
        
        // Primary trigger
        if (isRightController == (vr_righthanded->integer != 0))
        {
            if (triggerValue > triggerPressedThreshold) {
                IN_HandleActiveInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX, "PRIMARYTRIGGER", triggerValue, qfalse);
            } else if (triggerValue < triggerReleasedThreshold) {
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX, "PRIMARYTRIGGER", triggerValue, qfalse);
            }
        }

        // Off hand trigger
        if (isRightController != (vr_righthanded->integer != 0))
        {
            if (triggerValue > triggerPressedThreshold) {
                IN_HandleActiveInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX, "SECONDARYTRIGGER", triggerValue, qfalse);
            } else if (triggerValue < triggerReleasedThreshold) {
                IN_HandleInactiveInput(&controller->axisButtons, VR_TOUCH_AXIS_TRIGGER_INDEX, "SECONDARYTRIGGER", triggerValue, qfalse);
            }
        }

    }
}

static void IN_VRButtons( qboolean isRightController, uint32_t buttons )
{
	vrController_t* controller = isRightController == qtrue ? &rightController : &leftController;

    // Menu button
    if ((buttons & ovrButton_Enter) && !IN_InputActivated(&controller->buttons, ovrButton_Enter)) {
        IN_ActivateInput(&controller->buttons, ovrButton_Enter);
        Com_QueueEvent(in_vrEventTime, SE_KEY, K_ESCAPE, qtrue, 0, NULL);
    } else if (!(buttons & ovrButton_Enter) && IN_InputActivated(&controller->buttons, ovrButton_Enter)) {
        IN_DeactivateInput(&controller->buttons, ovrButton_Enter);
        Com_QueueEvent(in_vrEventTime, SE_KEY, K_ESCAPE, qfalse, 0, NULL);
    }

	if (isRightController == !vr_righthanded->integer)
    {
        if (buttons & ovrButton_GripTrigger) {
            IN_HandleActiveInput(&controller->buttons, ovrButton_GripTrigger, "SECONDARYGRIP", 0, qfalse);
        } else {
            IN_HandleInactiveInput(&controller->buttons, ovrButton_GripTrigger, "SECONDARYGRIP", 0, qfalse);
        }
	}
    else
    {
        if (buttons & ovrButton_GripTrigger) {
            IN_HandleActiveInput(&controller->buttons, ovrButton_GripTrigger, "PRIMARYGRIP", 0, qfalse);
        } else {
            IN_HandleInactiveInput(&controller->buttons, ovrButton_GripTrigger, "PRIMARYGRIP", 0, qfalse);
        }
    }

    if (isRightController == !vr_righthanded->integer)
    {
        if (buttons & ovrButton_LThumb) {
            if (!IN_InputActivated(&controller->buttons, ovrButton_LThumb)) {
                // Initiate position reset for fake 6DoF
                vr.realign = 3;
            }
            IN_HandleActiveInput(&controller->buttons, ovrButton_LThumb, "SECONDARYTHUMBSTICK", 0, qfalse);
        } else {
            IN_HandleInactiveInput(&controller->buttons, ovrButton_LThumb, "SECONDARYTHUMBSTICK", 0, qfalse);
        }
    }
    else
    {
        if (buttons & ovrButton_RThumb) {
            IN_HandleActiveInput(&controller->buttons, ovrButton_RThumb, "PRIMARYTHUMBSTICK", 0, qfalse);
        } else {
            IN_HandleInactiveInput(&controller->buttons, ovrButton_RThumb, "PRIMARYTHUMBSTICK", 0, qfalse);
        }
    }

    if (buttons & ovrButton_A) {
        if (cl.snap.ps.pm_flags & PMF_FOLLOW)
        {
            // Go back to free spectator mode if following player
            if (!IN_InputActivated(&controller->buttons, ovrButton_A)) {
                IN_ActivateInput(&controller->buttons, ovrButton_A);
                Cbuf_AddText("cmd team spectator\n");
            }
        }
        else
        {
            IN_HandleActiveInput(&controller->buttons, ovrButton_A, "A", 0, qfalse);
        }
    } else {
       IN_HandleInactiveInput(&controller->buttons, ovrButton_A, "A", 0, qfalse);
    }

    if (buttons & ovrButton_B) {
        IN_HandleActiveInput(&controller->buttons, ovrButton_B, "B", 0, qfalse);
    } else {
        IN_HandleInactiveInput(&controller->buttons, ovrButton_B, "B", 0, qfalse);
    }

    if (buttons & ovrButton_X) {
        if (cl.snap.ps.pm_flags & PMF_FOLLOW)
        {
            // Switch follow mode
            if (!IN_InputActivated(&controller->buttons, ovrButton_X)) {
                IN_ActivateInput(&controller->buttons, ovrButton_X);
                vr.follow_mode = (vr.follow_mode+1) % VRFM_NUM_FOLLOWMODES;
            }
        }
        else
        {
            IN_HandleActiveInput(&controller->buttons, ovrButton_X, "X", 0, qfalse);
        }
    } else {
        IN_HandleInactiveInput(&controller->buttons, ovrButton_X, "X", 0, qfalse);
    }

    if (buttons & ovrButton_Y) {
        IN_HandleActiveInput(&controller->buttons, ovrButton_Y, "Y", 0, qfalse);
    } else {
        IN_HandleInactiveInput(&controller->buttons, ovrButton_Y, "Y", 0, qfalse);
    }

}

void IN_VRInputFrame( void )
{
	if (controllerInit == qfalse) {
		memset(&leftController, 0, sizeof(leftController));
		memset(&rightController, 0, sizeof(rightController));
		controllerInit = qtrue;
	}

	ovrMobile* ovr = VR_GetEngine()->ovr;
	if (!ovr) {
		return;
	}

    ovrResult result;
	if (vr_extralatencymode != NULL &&
            vr_extralatencymode->integer) {
        result = vrapi_SetExtraLatencyMode(VR_GetEngine()->ovr, VRAPI_EXTRA_LATENCY_MODE_ON);
        assert(result == VRAPI_INITIALIZE_SUCCESS);
    }

	if (vr_refreshrate != NULL && vr_refreshrate->integer)
	{
		vrapi_SetDisplayRefreshRate(VR_GetEngine()->ovr, (float)vr_refreshrate->integer);
	}

    result = vrapi_SetClockLevels(VR_GetEngine()->ovr, 4, 4);
    assert(result == VRAPI_INITIALIZE_SUCCESS);

	vr.virtual_screen = VR_useScreenLayer();

    VR_processHaptics();

	//trigger frame tick for haptics
    VR_HapticEvent("frame_tick", 0, 0, 0, 0, 0);

    {
		// We extract Yaw, Pitch, Roll instead of directly using the orientation
		// to allow "additional" yaw manipulation with mouse/controller.
		const ovrQuatf quatHmd =  VR_GetEngine()->tracking.HeadPose.Pose.Orientation;
		const ovrVector3f positionHmd = VR_GetEngine()->tracking.HeadPose.Pose.Position;
		vec3_t rotation = {0, 0, 0};
		QuatToYawPitchRoll(quatHmd, rotation, vr.hmdorientation);
		VectorSet(vr.hmdposition, positionHmd.x, positionHmd.y + vr_heightAdjust->value, positionHmd.z);

		//Position
		VectorSubtract(vr.hmdposition_last, vr.hmdposition, vr.hmdposition_delta);

		//Keep this for our records
		VectorCopy(vr.hmdposition, vr.hmdposition_last);

		//Orientation
		VectorSubtract(vr.hmdorientation_last, vr.hmdorientation, vr.hmdorientation_delta);

		//Keep this for our records
		VectorCopy(vr.hmdorientation, vr.hmdorientation_last);

		// View yaw delta
		const float clientview_yaw = vr.clientviewangles[YAW] - vr.hmdorientation[YAW];
		vr.clientview_yaw_delta = vr.clientview_yaw_last - clientview_yaw;
		vr.clientview_yaw_last = clientview_yaw;
	}

	ovrInputCapabilityHeader capsHeader;
	uint32_t index = 0;
	for (;;) {
		ovrResult enumResult = vrapi_EnumerateInputDevices(ovr, index, &capsHeader);
		if (enumResult < 0) {
			break;
		}
		++index;
		
		if (capsHeader.Type != ovrControllerType_TrackedRemote) {
			continue;
		}

		ovrInputTrackedRemoteCapabilities caps;
		caps.Header = capsHeader;
		ovrResult capsResult = vrapi_GetInputDeviceCapabilities(ovr, &caps.Header);
		if (capsResult < 0) {
			continue;
		}

		ovrInputStateTrackedRemote state;
		state.Header.ControllerType = ovrControllerType_TrackedRemote;
		ovrResult stateResult = vrapi_GetCurrentInputState(ovr, capsHeader.DeviceID, &state.Header);
		if (stateResult < 0) {
			continue;
		}

		ovrTracking remoteTracking;
		stateResult = vrapi_GetInputTrackingState(ovr, capsHeader.DeviceID, VR_GetEngine()->predictedDisplayTime,
											 &remoteTracking);
		if (stateResult < 0) {
			continue;
		}

		qboolean isRight;
		vrController_t* controller;
		if (caps.ControllerCapabilities & ovrControllerCaps_LeftHand) {
			isRight = qfalse;
			controller = &leftController;
            controllerIDs[0] = capsHeader.DeviceID;
		} else if (caps.ControllerCapabilities & ovrControllerCaps_RightHand) {
			isRight = qtrue;
			controller = &rightController;
            controllerIDs[1] = capsHeader.DeviceID;
		}
		else {
			continue;
		}

        IN_VRButtons(isRight, state.Buttons);
		IN_VRController(isRight, remoteTracking);
		IN_VRJoystick(isRight, state.Joystick.x, state.Joystick.y);
		IN_VRTriggers(isRight, state.IndexTrigger);
	}

	lastframetime = in_vrEventTime;
	in_vrEventTime = Sys_Milliseconds( );
}

//#endif
