#if !defined(vr_clientinfo_h)
#define vr_clientinfo_h

#include "vr_base.h"

#define NUM_WEAPON_SAMPLES      10

typedef enum {
    VRFM_THIRDPERSON,
    VRFM_FIRSTPERSON
} followMode_t;

typedef struct {
    qboolean weapon_stabilised;
    qboolean weapon_zoomed;
    qboolean show_console;
    float weapon_zoomLevel;
    qboolean right_handed;
    qboolean virtual_screen;
    qboolean local_server; // used in bg_pmove.c
    followMode_t follow_mode;
    qboolean weapon_select;
    qboolean smooth_turning;
    qboolean weapon_wheel_active;

    int realign; // used to realign the fake 6DoF playspace in a multiplayer game

    int clientNum;
    vec3_t clientviewangles; //orientation in the client - we use this in the cgame

    vec3_t hmdposition;
    vec3_t hmdorigin; //used to recenter the mp fake 6DoF playspace
    vec3_t hmdposition_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdposition_delta;

    vec3_t hmdorientation;
    vec3_t hmdorientation_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdorientation_delta;
	
    vec3_t weaponangles;
    vec3_t calculated_weaponangles; //Calculated as the angle required to hit the point that the controller is pointing at, but coming from the view origin
    vec3_t weaponangles_last; // Don't use this, it is just for calculating delta!
    vec3_t weaponangles_delta;

    vec3_t weaponoffset;
    vec3_t weaponoffset_last[2];
    vec3_t weaponposition;

    vec3_t offhandangles;
    vec3_t offhandoffset;
    vec3_t offhandoffset_last[2];
    vec3_t offhandposition;

    float right_axis_x;
    float right_axis_y;

    //////////////////////////////////////
    //    Test stuff for weapon alignment
    //////////////////////////////////////

    char    test_name[256];
    float   test_scale;
    vec3_t  test_angles;
    vec3_t  test_offset;

} vr_clientinfo_t;

#endif //vr_clientinfo_h