#include "vr_base.h"
#include "vr_renderer.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "../VrApi/Include/VrApi_Types.h"

#include "vr_clientinfo.h"
#include "vr_types.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <VrApi.h>
#include <VrApi_Helpers.h>
#pragma clang diagnostic pop

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_GL_DEBUG 0
#define ENABLE_GL_DEBUG_VERBOSE 0
#if ENABLE_GL_DEBUG
#include <GLES3/gl32.h>
#endif

#define DEFAULT_SUPER_SAMPLING  1.1f

extern vr_clientinfo_t vr;

void APIENTRY VR_GLDebugLog(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_PERFORMANCE || ENABLE_GL_DEBUG_VERBOSE)
	{
		char typeStr[128];
		switch (type) {
			case GL_DEBUG_TYPE_ERROR: sprintf(typeStr, "ERROR"); break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: sprintf(typeStr, "DEPRECATED_BEHAVIOR"); break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: sprintf(typeStr, "UNDEFINED_BEHAVIOR"); break;
			case GL_DEBUG_TYPE_PORTABILITY: sprintf(typeStr, "PORTABILITY"); break;
			case GL_DEBUG_TYPE_PERFORMANCE: sprintf(typeStr, "PERFORMANCE"); break;
			case GL_DEBUG_TYPE_MARKER: sprintf(typeStr, "MARKER"); break;
			case GL_DEBUG_TYPE_PUSH_GROUP: sprintf(typeStr, "PUSH_GROUP"); break;
			case GL_DEBUG_TYPE_POP_GROUP: sprintf(typeStr, "POP_GROUP"); break;
			default: sprintf(typeStr, "OTHER"); break;
		}

		char severinityStr[128];
		switch (severity) {
			case GL_DEBUG_SEVERITY_HIGH: sprintf(severinityStr, "HIGH"); break;
			case GL_DEBUG_SEVERITY_MEDIUM: sprintf(severinityStr, "MEDIUM"); break;
			case GL_DEBUG_SEVERITY_LOW: sprintf(severinityStr, "LOW"); break;
			default: sprintf(severinityStr, "VERBOSE"); break;
		}

		Com_Printf("[%s] GL issue - %s: %s\n", severinityStr, typeStr, message);
	}
}

void VR_GetResolution(engine_t* engine, int *pWidth, int *pHeight)
{
	static int width = 0;
	static int height = 0;
	float superSampling = 0.0f;

	if (vr.superSampling == 0.0f) {
		vr.superSampling = Cvar_VariableValue("vr_superSampling");
		if (vr.superSampling != 0.0f) {
			Cbuf_AddText( "vid_restart\n" );
		}
	}

	if (vr.superSampling == 0.0f || VR_useScreenLayer()) {
		// SS value affects scaling in menu, therefore default is used when menu is displayed
		superSampling = DEFAULT_SUPER_SAMPLING;
	} else {
		superSampling = vr.superSampling;
	}

	if (engine)
	{
		*pWidth = width = vrapi_GetSystemPropertyInt(&engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) * superSampling;
		*pHeight = height = vrapi_GetSystemPropertyInt(&engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT) * superSampling;

		vr.fov_x = vrapi_GetSystemPropertyInt( &engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
		vr.fov_y = vrapi_GetSystemPropertyInt( &engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
	}
	else
	{
		//use cached values
		*pWidth = width;
		*pHeight = height;
	}
}

typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
GLenum target,
        GLenum attachment,
GLuint texture,
        GLint level,
GLint baseViewIndex,
        GLsizei numViews);

void VR_InitRenderer( engine_t* engine ) {
#if ENABLE_GL_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(VR_GLDebugLog, 0);
#endif

    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
            (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
                    "glFramebufferTextureMultiviewOVR");

	int eyeW, eyeH;
    VR_GetResolution(engine, &eyeW, &eyeH);
	
	//for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye) 
	{
		framebuffer_t* framebuffer = &engine->framebuffers;
		framebuffer->colorTexture = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D_ARRAY, GL_RGBA8,
			eyeW, eyeH, 1, 3);
		framebuffer->swapchainLength = vrapi_GetTextureSwapChainLength(framebuffer->colorTexture);
		framebuffer->depthBuffers = (GLuint*)malloc(framebuffer->swapchainLength * sizeof(GLuint));
		framebuffer->framebuffers = (GLuint*)malloc(framebuffer->swapchainLength * sizeof(GLuint));

		for (int index = 0; index < framebuffer->swapchainLength; ++index) {
			GLuint colorTexture;
			GLenum framebufferStatus;

			colorTexture = vrapi_GetTextureSwapChainHandle(framebuffer->colorTexture, index);
			glBindTexture(GL_TEXTURE_2D_ARRAY, colorTexture);
            GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

            glGenTextures(1, &framebuffer->depthBuffers[index]);
            glBindTexture(GL_TEXTURE_2D_ARRAY, framebuffer->depthBuffers[index]);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, eyeW, eyeH, 2);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			glGenFramebuffers(1, &framebuffer->framebuffers[index]);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->framebuffers[index]);

            glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				framebuffer->depthBuffers[index], 0, 0, 2);
            glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             colorTexture, 0, 0, 2);

			framebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
			assert(framebufferStatus == GL_FRAMEBUFFER_COMPLETE);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	}
}

void VR_DestroyRenderer( engine_t* engine ) {
	for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye)
	{
		if (engine->framebuffers.swapchainLength > 0) {
			glDeleteFramebuffers(engine->framebuffers.swapchainLength,
				engine->framebuffers.depthBuffers);
			free(engine->framebuffers.depthBuffers);
			free(engine->framebuffers.framebuffers);

			vrapi_DestroyTextureSwapChain(engine->framebuffers.colorTexture);

			memset(&engine->framebuffers, 0, sizeof(engine->framebuffers));
		}
	}

}


void VR_ReInitRenderer()
{
    VR_DestroyRenderer( VR_GetEngine() );
    VR_InitRenderer( VR_GetEngine() );
}


// Assumes landscape cylinder shape.
static ovrMatrix4f CylinderModelMatrix( const int texWidth, const int texHeight,
										const ovrVector3f translation,
										const float rotateYaw,
										const float rotatePitch,
										const float radius,
										const float density )
{
	const ovrMatrix4f scaleMatrix = ovrMatrix4f_CreateScale( radius, radius * (float)texHeight * VRAPI_PI / density, radius );
	const ovrMatrix4f transMatrix = ovrMatrix4f_CreateTranslation( translation.x, translation.y, translation.z );
	const ovrMatrix4f rotXMatrix = ovrMatrix4f_CreateRotation( rotateYaw, 0.0f, 0.0f );
	const ovrMatrix4f rotYMatrix = ovrMatrix4f_CreateRotation( 0.0f, rotatePitch, 0.0f );

	const ovrMatrix4f m0 = ovrMatrix4f_Multiply( &transMatrix, &scaleMatrix );
	const ovrMatrix4f m1 = ovrMatrix4f_Multiply( &rotXMatrix, &m0 );
	const ovrMatrix4f m2 = ovrMatrix4f_Multiply( &rotYMatrix, &m1 );

	return m2;
}

extern cvar_t	*vr_screen_dist;

ovrLayerCylinder2 BuildCylinderLayer(engine_t* engine, const int textureWidth, const int textureHeight,
	const ovrTracking2 * tracking, float rotatePitch )
{
	ovrLayerCylinder2 layer = vrapi_DefaultLayerCylinder2();

	const float fadeLevel = 1.0f;
	layer.Header.ColorScale.x =
		layer.Header.ColorScale.y =
		layer.Header.ColorScale.z =
		layer.Header.ColorScale.w = fadeLevel;
	layer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
	layer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;

	//layer.Header.Flags = VRAPI_FRAME_LAYER_FLAG_CLIP_TO_TEXTURE_RECT;

	layer.HeadPose = tracking->HeadPose;

	const float density = 4500.0f;
	const float rotateYaw = 0.0f;
	const float radius = 12.0f;
	const float distance = -16.0f;

	const ovrVector3f translation = { 0.0f, 1.0f, distance };

	ovrMatrix4f cylinderTransform = 
		CylinderModelMatrix( textureWidth, textureHeight, translation,
			rotateYaw, rotatePitch, radius, density );

	const float circScale = density * 0.5f / textureWidth;
	const float circBias = -circScale * ( 0.5f * ( 1.0f - 1.0f / circScale ) );

	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		ovrMatrix4f modelViewMatrix = ovrMatrix4f_Multiply( &tracking->Eye[eye].ViewMatrix, &cylinderTransform );
		layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_Inverse( &modelViewMatrix );
		layer.Textures[eye].ColorSwapChain = engine->framebuffers.colorTexture;
		layer.Textures[eye].SwapChainIndex = engine->framebuffers.swapchainIndex;

		// Texcoord scale and bias is just a representation of the aspect ratio. The positioning
		// of the cylinder is handled entirely by the TexCoordsFromTanAngles matrix.

		const float texScaleX = circScale;
		const float texBiasX = circBias;
		const float texScaleY = -0.5f;
		const float texBiasY = texScaleY * ( 0.5f * ( 1.0f - ( 1.0f / texScaleY ) ) );

		layer.Textures[eye].TextureMatrix.M[0][0] = texScaleX;
		layer.Textures[eye].TextureMatrix.M[0][2] = texBiasX;
		layer.Textures[eye].TextureMatrix.M[1][1] = texScaleY;
		layer.Textures[eye].TextureMatrix.M[1][2] = -texBiasY;

		layer.Textures[eye].TextureRect.width = 1.0f;
		layer.Textures[eye].TextureRect.height = 1.0f;
	}

	return layer;
}

void VR_ClearFrameBuffer( GLuint frameBuffer, int width, int height)
{
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer );

    glEnable( GL_SCISSOR_TEST );
    glViewport( 0, 0, width, height );

	if (Cvar_VariableIntegerValue("vr_thirdPersonSpectator"))
	{
		//Blood red.. ish
		glClearColor( 0.12f, 0.0f, 0.05f, 1.0f );
	}
	else
	{
		//Black
		glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	}

    glScissor( 0, 0, width, height );
    glClear( GL_COLOR_BUFFER_BIT );

    glScissor( 0, 0, 0, 0 );
    glDisable( GL_SCISSOR_TEST );

    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
}

void VR_DrawFrame( engine_t* engine ) {

	if (!engine->ovr)
	{
		return;
	}

	++engine->frameIndex;
	engine->predictedDisplayTime = vrapi_GetPredictedDisplayTime(engine->ovr, engine->frameIndex);
	engine->tracking = vrapi_GetPredictedTracking2(engine->ovr, engine->predictedDisplayTime);

	float fov_y = vrapi_GetSystemPropertyInt( engine->ovr, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
	float fov_x = vrapi_GetSystemPropertyInt( engine->ovr, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);

	if (vr.weapon_zoomed) {
		vr.weapon_zoomLevel += 0.05;
		if (vr.weapon_zoomLevel > 2.5f)
            vr.weapon_zoomLevel = 2.5f;
	}
	else {
	    //Zoom back out quicker
        vr.weapon_zoomLevel -= 0.25f;
		if (vr.weapon_zoomLevel < 1.0f)
            vr.weapon_zoomLevel = 1.0f;
	}

	const ovrMatrix4f projectionMatrix = ovrMatrix4f_CreateProjectionFov(
			fov_x / vr.weapon_zoomLevel, fov_y / vr.weapon_zoomLevel, 0.0f, 0.0f, 1.0f, 0.0f );

	//Projection used for drawing HUD models etc
	const ovrMatrix4f monoVRMatrix = ovrMatrix4f_CreateProjectionFov(
			30.0f, 30.0f, 0.0f, 0.0f, 1.0f, 0.0f );

    int eyeW, eyeH;
    VR_GetResolution(engine, &eyeW, &eyeH);

    if (VR_useScreenLayer() ||
			(cl.snap.ps.pm_flags & PMF_FOLLOW && vr.follow_mode == VRFM_FIRSTPERSON))
	{
		static ovrLayer_Union2 cylinderLayer;
		memset( &cylinderLayer, 0, sizeof( ovrLayer_Union2 ) );

		// Add a simple cylindrical layer
		cylinderLayer.Cylinder =
				BuildCylinderLayer(engine, eyeW, eyeW * 0.75f, &engine->tracking, radians(vr.menuYaw) );

		const ovrLayerHeader2* layers[] = {
			&cylinderLayer.Header
		};

		// Set up the description for this frame.
		ovrSubmitFrameDescription2 frameDesc = { 0 };
		frameDesc.Flags = 0;
		frameDesc.SwapInterval = 1;
		frameDesc.FrameIndex = engine->frameIndex;
		frameDesc.DisplayTime = engine->predictedDisplayTime;
		frameDesc.LayerCount = 1;
		frameDesc.Layers = layers;

        re.SetVRHeadsetParms(&projectionMatrix, &monoVRMatrix,
                             engine->framebuffers.framebuffers[engine->framebuffers.swapchainIndex]);

		VR_ClearFrameBuffer(engine->framebuffers.framebuffers[engine->framebuffers.swapchainIndex], eyeW, eyeH);

		Com_Frame();

		// Clear the alpha channel, other way VR API would not transfer the framebuffer fully
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		engine->framebuffers.swapchainIndex = (engine->framebuffers.swapchainIndex + 1) %
			engine->framebuffers.swapchainLength;

		// Hand over the eye images to the time warp.
		vrapi_SubmitFrame2(engine->ovr, &frameDesc);		
	}
	else
	{
		vr.menuYaw = vr.hmdorientation[YAW];

		ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
		layer.HeadPose = engine->tracking.HeadPose;

        const ovrMatrix4f defaultProjection = ovrMatrix4f_CreateProjectionFov(
                fov_x, fov_y, 0.0f, 0.0f, 1.0f, 0.0f );


        for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye) {
			layer.Textures[eye].ColorSwapChain = engine->framebuffers.colorTexture;
			layer.Textures[eye].SwapChainIndex = engine->framebuffers.swapchainIndex;
			layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&defaultProjection);
		}
		layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

        VR_ClearFrameBuffer(engine->framebuffers.framebuffers[engine->framebuffers.swapchainIndex], eyeW, eyeH);

		re.SetVRHeadsetParms(&projectionMatrix, &monoVRMatrix,
							 engine->framebuffers.framebuffers[engine->framebuffers.swapchainIndex]);

		Com_Frame();

		engine->framebuffers.swapchainIndex = (engine->framebuffers.swapchainIndex + 1) %
			engine->framebuffers.swapchainLength;

		const ovrLayerHeader2* layers[] = {
			&layer.Header
		};

		ovrSubmitFrameDescription2 frameDesc = { 0 };
		frameDesc.Flags = 0;
		frameDesc.SwapInterval = 1;
		frameDesc.FrameIndex = engine->frameIndex;
		frameDesc.DisplayTime = engine->predictedDisplayTime;
		frameDesc.LayerCount = 1;
		frameDesc.Layers = layers;

		vrapi_SubmitFrame2(engine->ovr, &frameDesc);
	}

}
