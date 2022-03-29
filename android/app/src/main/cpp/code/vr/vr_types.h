#ifndef __VR_TYPES
#define __VR_TYPES

#ifdef USE_LOCAL_HEADERS
#	include "SDL_opengl.h"
#	include "SDL_opengles2.h"
#else
#	include <SDL_opengl.h>
#	include <SDL_opengles2.h>
#endif

#include <jni.h>

typedef struct {
	JavaVM* Vm;
	jobject ActivityObject;
	JNIEnv* Env;
} ovrJava;

typedef struct {
	int swapchainLength;
	int swapchainIndex;
	//TODO:ovrTextureSwapChain* colorTexture;
	GLuint* depthBuffers;
	GLuint* framebuffers;
} framebuffer_t;

typedef struct {
	uint64_t frameIndex;
	//TODO:ovrMobile* ovr;
	ovrJava java;
	double predictedDisplayTime;
	//TODO:ovrTracking2 tracking;
	framebuffer_t framebuffers[2];
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
