#ifndef __VR_TYPES
#define __VR_TYPES


#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <VrApi.h>
#pragma clang diagnostic pop

typedef struct {
	int swapchainLength;
	int swapchainIndex;
	ovrTextureSwapChain* colorTexture;
	GLuint* depthBuffers;
	GLuint* framebuffers;
} framebuffer_t;

typedef struct {
	uint64_t frameIndex;
	ovrMobile* ovr;
	ovrJava java;
	double predictedDisplayTime;
	ovrTracking2 tracking;
	framebuffer_t framebuffers;//[VRAPI_FRAME_LAYER_EYE_MAX];
} engine_t;

typedef enum {
	WS_CONTROLLER,
	WS_HMD,
	WS_ALTKEY,
	WS_PREVNEXT
} weaponSelectorType_t;

typedef enum {
    VRFM_THIRDPERSON_1,		//Camera will auto move to keep up with player
	VRFM_THIRDPERSON_2,		//Camera is completely free movement with the thumbstick
    VRFM_FIRSTPERSON,		//Obvious isn't it?..
    VRFM_NUM_FOLLOWMODES,

	VRFM_QUERY		= 99	//Used to query which mode is active
} vrFollowMode_t;

#endif
