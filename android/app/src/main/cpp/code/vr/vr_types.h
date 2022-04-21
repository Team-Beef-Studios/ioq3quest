#ifndef __VR_TYPES
#define __VR_TYPES

#ifdef USE_LOCAL_HEADERS
#	include "SDL_opengl.h"
#	include "SDL_opengles2.h"
#else
#	include <SDL_opengl.h>
#	include <SDL_opengles2.h>
#endif

//OpenXR
#define XR_EYES_COUNT 2
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <jni.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>

typedef struct {
	JavaVM* Vm;
	jobject ActivityObject;
	JNIEnv* Env;
} ovrJava;

typedef struct {
	XrSwapchain Handle;
	uint32_t Width;
	uint32_t Height;
} ovrSwapChain;

typedef struct {
	int Width;
	int Height;
	int Multisamples;
	uint32_t TextureSwapChainLength;
	uint32_t TextureSwapChainIndex;
	ovrSwapChain ColorSwapChain;
	XrSwapchainImageOpenGLESKHR* ColorSwapChainImage;
	GLuint* DepthBuffers;
	GLuint* FrameBuffers;
} ovrFramebuffer;

typedef struct {
	ovrFramebuffer FrameBuffer[XR_EYES_COUNT];
} ovrRenderer;

typedef struct {
	XrMatrix4x4f ViewMatrix[XR_EYES_COUNT];
	XrMatrix4x4f ProjectionMatrix[XR_EYES_COUNT];
} ovrSceneMatrices;

typedef struct ovrMatrix4f_ {
    float M[4][4];
} ovrMatrix4f;

typedef struct {
	uint64_t frameIndex;
	ovrJava java;
	ovrRenderer renderer;
	XrInstance instance;
	XrSession session;
	XrSystemId systemId;
	XrSpace stageSpace;
    GLboolean sessionActive;
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

//ovrFramebuffer
void ovrFramebuffer_Clear(ovrFramebuffer* frameBuffer);
GLboolean ovrFramebuffer_Create(
		XrSession session,
		ovrFramebuffer* frameBuffer,
		const GLenum colorFormat,
		const int width,
		const int height,
		const int multisamples);
void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetNone();
void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer);

//ovrRenderer
void ovrRenderer_Clear(ovrRenderer* renderer);
void ovrRenderer_Create(
		XrSession session,
		ovrRenderer* renderer,
		int suggestedEyeTextureWidth,
		int suggestedEyeTextureHeight);
void ovrRenderer_Destroy(ovrRenderer* renderer);
void ovrRenderer_SetFoveation(
		XrInstance* instance,
		XrSession* session,
		ovrRenderer* renderer,
		XrFoveationLevelFB level,
		float verticalOffset,
		XrFoveationDynamicFB dynamic);

//ovrMatrix4f
ovrMatrix4f ovrMatrix4f_CreateProjection(
		const float minX,
		const float maxX,
		float const minY,
		const float maxY,
		const float nearZ,
		const float farZ);
ovrMatrix4f ovrMatrix4f_CreateProjectionFov(
		const float fovDegreesX,
		const float fovDegreesY,
		const float offsetX,
		const float offsetY,
		const float nearZ,
		const float farZ);

#endif
