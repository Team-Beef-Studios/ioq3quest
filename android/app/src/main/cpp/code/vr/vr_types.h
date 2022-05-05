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

#define ALOGE(...) printf(__VA_ARGS__)
#define ALOGV(...) printf(__VA_ARGS__)

typedef union {
    XrCompositionLayerProjection Projection;
    XrCompositionLayerCylinderKHR Cylinder;
} ovrCompositorLayer_Union;

enum { ovrMaxLayerCount = 1 };
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
    uint32_t TextureSwapChainLength;
    uint32_t TextureSwapChainIndex;
    ovrSwapChain ColorSwapChain;
    XrSwapchainImageOpenGLESKHR* ColorSwapChainImage;
    GLuint* DepthBuffers;
    GLuint* FrameBuffers;
} ovrFramebuffer;

typedef struct {
    ovrFramebuffer FrameBuffer;
} ovrRenderer;

typedef struct {
    GLboolean Active;
    XrPosef Pose;
} ovrTrackedController;

typedef struct {
    GLboolean Focused;

    XrInstance Instance;
    XrSession Session;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[ovrMaxNumEyes];
    XrSystemId SystemId;
    XrSpace HeadSpace;
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
    ovrTrackedController TrackedController[2];
} ovrApp;


typedef struct {
    float M[4][4];
} ovrMatrix4f;

typedef enum ovrButton_ {
    ovrButton_A = 0x00000001, // Set for trigger pulled on the Gear VR and Go Controllers
    ovrButton_B = 0x00000002,
    ovrButton_RThumb = 0x00000004,
    ovrButton_RShoulder = 0x00000008,

    ovrButton_X = 0x00000100,
    ovrButton_Y = 0x00000200,
    ovrButton_LThumb = 0x00000400,
    ovrButton_LShoulder = 0x00000800,

    ovrButton_Up = 0x00010000,
    ovrButton_Down = 0x00020000,
    ovrButton_Left = 0x00040000,
    ovrButton_Right = 0x00080000,
    ovrButton_Enter = 0x00100000, //< Set for touchpad click on the Go Controller, menu
    // button on Left Quest Controller
    ovrButton_Back = 0x00200000, //< Back button on the Go Controller (only set when
    // a short press comes up)
    ovrButton_GripTrigger = 0x04000000, //< grip trigger engaged
    ovrButton_Trigger = 0x20000000, //< Index Trigger engaged
    ovrButton_Joystick = 0x80000000, //< Click of the Joystick

    ovrButton_EnumSize = 0x7fffffff
} ovrButton;

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
GLboolean ovrApp_HandleXrEvents(ovrApp* app);

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

void ovrTrackedController_Clear(ovrTrackedController* controller);

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
