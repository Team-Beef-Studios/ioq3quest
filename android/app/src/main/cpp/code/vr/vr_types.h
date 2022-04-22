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

#define MATH_PI 3.14159265358979323846f

#if !defined(GL_EXT_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(
GLenum target,
GLsizei samples,
GLenum internalformat,
GLsizei width,
GLsizei height);
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(
GLenum target,
GLenum attachment,
GLenum textarget,
GLuint texture,
GLint level,
GLsizei samples);
#endif

#define ALOGE(...) printf(__VA_ARGS__)
#define ALOGV(...) printf(__VA_ARGS__)

typedef union {
    XrCompositionLayerProjection Projection;
    XrCompositionLayerCylinderKHR Cylinder;
} ovrCompositorLayer_Union;

enum { ovrMaxLayerCount = 16 };
enum { ovrMaxNumEyes = 2 };

#define GL(func) func;
#define OXR(func) func;

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
    ovrFramebuffer FrameBuffer[ovrMaxNumEyes];
} ovrRenderer;

typedef struct {
    GLboolean Focused;

    XrInstance Instance;
    XrSession Session;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[ovrMaxNumEyes];
    XrSystemId SystemId;
    XrSpace HeadSpace;
    XrSpace LocalSpace;
    XrSpace StageSpace;
    XrSpace FakeStageSpace;
    XrSpace CurrentSpace;
    GLboolean SessionActive;

    float* SupportedDisplayRefreshRates;
    uint32_t RequestedDisplayRefreshRateIndex;
    uint32_t NumSupportedDisplayRefreshRates;
    PFN_xrGetDisplayRefreshRateFB pfnGetDisplayRefreshRate;
    PFN_xrRequestDisplayRefreshRateFB pfnRequestDisplayRefreshRate;

    int SwapInterval;
    // These threads will be marked as performance threads.
    int MainThreadTid;
    int RenderThreadTid;
    ovrCompositorLayer_Union Layers[ovrMaxLayerCount];
    int LayerCount;

    GLboolean TouchPadDownLastFrame;
    ovrRenderer Renderer;
} ovrApp;


typedef struct {
    float M[4][4];
} ovrMatrix4f;

typedef struct {
	uint64_t frameIndex;
	ovrApp appState;
	ovrJava java;
	float predictedDisplayTime;
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

void ovrApp_Clear(ovrApp* app);
void ovrApp_Destroy(ovrApp* app);
void ovrApp_HandleXrEvents(ovrApp* app);

void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetNone();

void ovrRenderer_Create(
		XrSession session,
		ovrRenderer* renderer,
		int suggestedEyeTextureWidth,
		int suggestedEyeTextureHeight);
void ovrRenderer_Destroy(ovrRenderer* renderer);

ovrMatrix4f ovrMatrix4f_Multiply(const ovrMatrix4f* a, const ovrMatrix4f* b);
ovrMatrix4f ovrMatrix4f_CreateRotation(const float radiansX, const float radiansY, const float radiansZ);
ovrMatrix4f ovrMatrix4f_CreateFromQuaternion(const XrQuaternionf* q);
ovrMatrix4f ovrMatrix4f_CreateProjectionFov(
		const float fovDegreesX,
		const float fovDegreesY,
		const float offsetX,
		const float offsetY,
		const float nearZ,
		const float farZ);

XrVector4f XrVector4f_MultiplyMatrix4f(const ovrMatrix4f* a, const XrVector4f* v);

#endif
