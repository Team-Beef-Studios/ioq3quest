#include "vr_base.h"
#include "vr_renderer.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

#include "vr_clientinfo.h"
#include "vr_types.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_GL_DEBUG 0
#define ENABLE_GL_DEBUG_VERBOSE 0
#if ENABLE_GL_DEBUG
#include <GLES3/gl32.h>
#endif

extern vr_clientinfo_t vr;

XrView* projections;
GLboolean stageSupported = GL_FALSE;

void VR_UpdateStageBounds(ovrApp* pappState) {
    XrExtent2Df stageBounds = {};

    XrResult result;
    OXR(result = xrGetReferenceSpaceBoundsRect(
            pappState->Session, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
    if (result != XR_SUCCESS) {
        ALOGV("Stage bounds query failed: using small defaults");
        stageBounds.width = 1.0f;
        stageBounds.height = 1.0f;

        pappState->CurrentSpace = pappState->FakeStageSpace;
    }

    ALOGV("Stage bounds: width = %f, depth %f", stageBounds.width, stageBounds.height);
}


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
	
	if (engine)
	{
        // Enumerate the viewport configurations.
        uint32_t viewportConfigTypeCount = 0;
        OXR(xrEnumerateViewConfigurations(
                engine->appState.Instance, engine->appState.SystemId, 0, &viewportConfigTypeCount, NULL));

        XrViewConfigurationType* viewportConfigurationTypes =
                (XrViewConfigurationType*)malloc(viewportConfigTypeCount * sizeof(XrViewConfigurationType));

        OXR(xrEnumerateViewConfigurations(
                engine->appState.Instance,
                engine->appState.SystemId,
                viewportConfigTypeCount,
                &viewportConfigTypeCount,
                viewportConfigurationTypes));

        ALOGV("Available Viewport Configuration Types: %d", viewportConfigTypeCount);

        for (uint32_t i = 0; i < viewportConfigTypeCount; i++) {
            const XrViewConfigurationType viewportConfigType = viewportConfigurationTypes[i];

            ALOGV(
                    "Viewport configuration type %d : %s",
                    viewportConfigType,
                    viewportConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO ? "Selected" : "");

            XrViewConfigurationProperties viewportConfig;
            viewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
            OXR(xrGetViewConfigurationProperties(
                    engine->appState.Instance, engine->appState.SystemId, viewportConfigType, &viewportConfig));
            ALOGV(
                    "FovMutable=%s ConfigurationType %d",
                    viewportConfig.fovMutable ? "true" : "false",
                    viewportConfig.viewConfigurationType);

            uint32_t viewCount;
            OXR(xrEnumerateViewConfigurationViews(
                    engine->appState.Instance, engine->appState.SystemId, viewportConfigType, 0, &viewCount, NULL));

            if (viewCount > 0) {
                XrViewConfigurationView* elements =
                        (XrViewConfigurationView*)malloc(viewCount * sizeof(XrViewConfigurationView));

                for (uint32_t e = 0; e < viewCount; e++) {
                    elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                    elements[e].next = NULL;
                }

                OXR(xrEnumerateViewConfigurationViews(
                        engine->appState.Instance,
                        engine->appState.SystemId,
                        viewportConfigType,
                        viewCount,
                        &viewCount,
                        elements));

                // Cache the view config properties for the selected config type.
                if (viewportConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                    assert(viewCount == ovrMaxNumEyes);
                    for (uint32_t e = 0; e < viewCount; e++) {
                        engine->appState.ViewConfigurationView[e] = elements[e];
                    }
                }

                free(elements);
            } else {
                ALOGE("Empty viewport configuration type: %d", viewCount);
            }
        }

        free(viewportConfigurationTypes);

        *pWidth = width = engine->appState.ViewConfigurationView[0].recommendedImageRectWidth;
        *pHeight = height = engine->appState.ViewConfigurationView[0].recommendedImageRectHeight;
        //TODO:
        /*
        vr.fov_x = vrapi_GetSystemPropertyInt( &engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
        vr.fov_y = vrapi_GetSystemPropertyInt( &engine->java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
         */
	}
	else
	{
		//use cached values
		*pWidth = width;
		*pHeight = height;
	}
}

void VR_InitRenderer( engine_t* engine ) {
#if ENABLE_GL_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(VR_GLDebugLog, 0);
#endif

	int eyeW, eyeH;
    VR_GetResolution(engine, &eyeW, &eyeH);

    // Get the viewport configuration info for the chosen viewport configuration type.
    engine->appState.ViewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;

    OXR(xrGetViewConfigurationProperties(
            engine->appState.Instance, engine->appState.SystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, &engine->appState.ViewportConfig));

    // Get the supported display refresh rates for the system.
    {
        PFN_xrEnumerateDisplayRefreshRatesFB pfnxrEnumerateDisplayRefreshRatesFB = NULL;
        OXR(xrGetInstanceProcAddr(
                engine->appState.Instance,
                "xrEnumerateDisplayRefreshRatesFB",
                (PFN_xrVoidFunction*)(&pfnxrEnumerateDisplayRefreshRatesFB)));

        OXR(pfnxrEnumerateDisplayRefreshRatesFB(
                engine->appState.Session, 0, &engine->appState.NumSupportedDisplayRefreshRates, NULL));

        engine->appState.SupportedDisplayRefreshRates =
                (float*)malloc(engine->appState.NumSupportedDisplayRefreshRates * sizeof(float));
        OXR(pfnxrEnumerateDisplayRefreshRatesFB(
                engine->appState.Session,
                engine->appState.NumSupportedDisplayRefreshRates,
                &engine->appState.NumSupportedDisplayRefreshRates,
                engine->appState.SupportedDisplayRefreshRates));
        ALOGV("Supported Refresh Rates:");
        for (uint32_t i = 0; i < engine->appState.NumSupportedDisplayRefreshRates; i++) {
            ALOGV("%d:%f", i, engine->appState.SupportedDisplayRefreshRates[i]);
        }

        OXR(xrGetInstanceProcAddr(
                engine->appState.Instance,
                "xrGetDisplayRefreshRateFB",
                (PFN_xrVoidFunction*)(&engine->appState.pfnGetDisplayRefreshRate)));

        float currentDisplayRefreshRate = 0.0f;
        OXR(engine->appState.pfnGetDisplayRefreshRate(engine->appState.Session, &currentDisplayRefreshRate));
        ALOGV("Current System Display Refresh Rate: %f", currentDisplayRefreshRate);

        OXR(xrGetInstanceProcAddr(
                engine->appState.Instance,
                "xrRequestDisplayRefreshRateFB",
                (PFN_xrVoidFunction*)(&engine->appState.pfnRequestDisplayRefreshRate)));

        // Test requesting the system default.
        OXR(engine->appState.pfnRequestDisplayRefreshRate(engine->appState.Session, 0.0f));
        ALOGV("Requesting system default display refresh rate");
    }

    uint32_t numOutputSpaces = 0;
    OXR(xrEnumerateReferenceSpaces(engine->appState.Session, 0, &numOutputSpaces, NULL));

    XrReferenceSpaceType* referenceSpaces =
            (XrReferenceSpaceType*)malloc(numOutputSpaces * sizeof(XrReferenceSpaceType));

    OXR(xrEnumerateReferenceSpaces(
            engine->appState.Session, numOutputSpaces, &numOutputSpaces, referenceSpaces));

    for (uint32_t i = 0; i < numOutputSpaces; i++) {
        if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            stageSupported = GL_TRUE;
            break;
        }
    }

    free(referenceSpaces);

    // Create a space to the first path
    XrReferenceSpaceCreateInfo spaceCreateInfo = {};
    spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    OXR(xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.HeadSpace));

    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    OXR(xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.LocalSpace));

    // Create a default stage space to use if SPACE_TYPE_STAGE is not
    // supported, or calls to xrGetReferenceSpaceBoundsRect fail.
    {
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceCreateInfo.poseInReferenceSpace.position.y = -1.6750f;
        OXR(xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.FakeStageSpace));
        ALOGV("Created fake stage space from local space with offset");
        engine->appState.CurrentSpace = engine->appState.FakeStageSpace;
    }

    if (stageSupported) {
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
        OXR(xrCreateReferenceSpace(engine->appState.Session, &spaceCreateInfo, &engine->appState.StageSpace));
        ALOGV("Created stage space");
        engine->appState.CurrentSpace = engine->appState.StageSpace;
    }

    projections = (XrView*)(malloc(ovrMaxNumEyes * sizeof(XrView)));

    ovrRenderer_Create(
            engine->appState.Session,
            &engine->appState.Renderer,
            engine->appState.ViewConfigurationView[0].recommendedImageRectWidth,
            engine->appState.ViewConfigurationView[0].recommendedImageRectHeight);
}

void VR_DestroyRenderer( engine_t* engine ) {
    ovrRenderer_Destroy(&engine->appState.Renderer);
    free(projections);
}


//TODO:
/*
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
		layer.Textures[eye].ColorSwapChain = engine->framebuffers[eye].colorTexture;
		layer.Textures[eye].SwapChainIndex = engine->framebuffers[eye].swapchainIndex;

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
*/

void VR_DrawFrame( engine_t* engine ) {
	float fov_y = 90; //TODO:
	float fov_x = 90; //TODO:

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
	re.SetVRHeadsetParms(projectionMatrix.M,
						 engine->appState.Renderer.FrameBuffer[0].FrameBuffers[engine->appState.Renderer.FrameBuffer[0].TextureSwapChainIndex],
						 engine->appState.Renderer.FrameBuffer[1].FrameBuffers[engine->appState.Renderer.FrameBuffer[1].TextureSwapChainIndex]);

    GLboolean stageBoundsDirty = GL_TRUE;
    ovrApp_HandleXrEvents(&engine->appState);
    if (engine->appState.SessionActive == GL_FALSE) {
        return;
    }

    if (stageBoundsDirty) {
        VR_UpdateStageBounds(&engine->appState);
        stageBoundsDirty = GL_FALSE;
    }

    // NOTE: OpenXR does not use the concept of frame indices. Instead,
    // XrWaitFrame returns the predicted display time.
    XrFrameWaitInfo waitFrameInfo = {};
    waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
    waitFrameInfo.next = NULL;

    XrFrameState frameState = {};
    frameState.type = XR_TYPE_FRAME_STATE;
    frameState.next = NULL;

    OXR(xrWaitFrame(engine->appState.Session, &waitFrameInfo, &frameState));

    // Get the HMD pose, predicted for the middle of the time period during which
    // the new eye images will be displayed. The number of frames predicted ahead
    // depends on the pipeline depth of the engine and the synthesis rate.
    // The better the prediction, the less black will be pulled in at the edges.
    XrFrameBeginInfo beginFrameDesc = {};
    beginFrameDesc.type = XR_TYPE_FRAME_BEGIN_INFO;
    beginFrameDesc.next = NULL;
    OXR(xrBeginFrame(engine->appState.Session, &beginFrameDesc));

    XrSpaceLocation loc = {};
    loc.type = XR_TYPE_SPACE_LOCATION;
    OXR(xrLocateSpace(
            engine->appState.HeadSpace, engine->appState.CurrentSpace, frameState.predictedDisplayTime, &loc));
    XrPosef xfStageFromHead = loc.pose;
    OXR(xrLocateSpace(
            engine->appState.HeadSpace, engine->appState.LocalSpace, frameState.predictedDisplayTime, &loc));

    XrViewLocateInfo projectionInfo = {};
    projectionInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
    projectionInfo.viewConfigurationType = engine->appState.ViewportConfig.viewConfigurationType;
    projectionInfo.displayTime = frameState.predictedDisplayTime;
    projectionInfo.space = engine->appState.HeadSpace;

    XrViewState viewState = {XR_TYPE_VIEW_STATE, NULL};

    uint32_t projectionCapacityInput = ovrMaxNumEyes;
    uint32_t projectionCountOutput = projectionCapacityInput;

    OXR(xrLocateViews(
            engine->appState.Session,
            &projectionInfo,
            &viewState,
            projectionCapacityInput,
            &projectionCountOutput,
            projections));
    //

    ovrSceneMatrices sceneMatrices;
    XrPosef viewTransform[2];

    for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
        XrPosef xfHeadFromEye = projections[eye].pose;
        XrPosef xfStageFromEye = XrPosef_Multiply(xfStageFromHead, xfHeadFromEye);
        viewTransform[eye] = XrPosef_Inverse(xfStageFromEye);

        sceneMatrices.ViewMatrix[eye] =
                XrMatrix4x4f_CreateFromRigidTransform(&viewTransform[eye]);

        const XrFovf fov = projections[eye].fov;
        XrMatrix4x4f_CreateProjectionFov(
                &sceneMatrices.ProjectionMatrix[eye],
                fov.angleLeft,
                fov.angleRight,
                fov.angleUp,
                fov.angleDown,
                0.1f,
                0.0f);
    }

    // Set-up the compositor layers for this frame.
    // NOTE: Multiple independent layers are allowed, but they need to be added
    // in a depth consistent order.

    XrCompositionLayerProjectionView projection_layer_elements[2] = {};

    engine->appState.LayerCount = 0;
    memset(engine->appState.Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);
    GLboolean shouldRenderWorldLayer = GL_TRUE;

    // Render the world-view layer
    if (shouldRenderWorldLayer) {

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer* frameBuffer = &engine->appState.Renderer.FrameBuffer[eye];

            ovrFramebuffer_Acquire(frameBuffer);

            // Set the current framebuffer.
            ovrFramebuffer_SetCurrent(frameBuffer);

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
            glClear( GL_COLOR_BUFFER_BIT );

            Com_Frame();

            ovrFramebuffer_Resolve(frameBuffer);

            ovrFramebuffer_Release(frameBuffer);
        }

        ovrFramebuffer_SetNone();

        XrCompositionLayerProjection projection_layer = {};
        projection_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
        projection_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        projection_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        projection_layer.space = engine->appState.CurrentSpace;
        projection_layer.viewCount = ovrMaxNumEyes;
        projection_layer.views = projection_layer_elements;

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer* frameBuffer = &engine->appState.Renderer.FrameBuffer[eye];

            memset(
                    &projection_layer_elements[eye], 0, sizeof(XrCompositionLayerProjectionView));
            projection_layer_elements[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;

            projection_layer_elements[eye].pose = XrPosef_Inverse(viewTransform[eye]);
            projection_layer_elements[eye].fov = projections[eye].fov;

            memset(&projection_layer_elements[eye].subImage, 0, sizeof(XrSwapchainSubImage));
            projection_layer_elements[eye].subImage.swapchain =
                    frameBuffer->ColorSwapChain.Handle;
            projection_layer_elements[eye].subImage.imageRect.offset.x = 0;
            projection_layer_elements[eye].subImage.imageRect.offset.y = 0;
            projection_layer_elements[eye].subImage.imageRect.extent.width =
                    frameBuffer->ColorSwapChain.Width;
            projection_layer_elements[eye].subImage.imageRect.extent.height =
                    frameBuffer->ColorSwapChain.Height;
            projection_layer_elements[eye].subImage.imageArrayIndex = 0;
        }

        engine->appState.Layers[engine->appState.LayerCount++].Projection = projection_layer;
    }

    // Compose the layers for this frame.
    const XrCompositionLayerBaseHeader* layers[ovrMaxLayerCount] = {};
    for (int i = 0; i < engine->appState.LayerCount; i++) {
        layers[i] = (const XrCompositionLayerBaseHeader*)&engine->appState.Layers[i];
    }

    XrFrameEndInfo endFrameInfo = {};
    endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
    endFrameInfo.displayTime = frameState.predictedDisplayTime;
    endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endFrameInfo.layerCount = engine->appState.LayerCount;
    endFrameInfo.layers = layers;

    OXR(xrEndFrame(engine->appState.Session, &endFrameInfo));

	//TODO
	/*
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

		const framebuffer_t* framebuffers = engine->framebuffers;

        re.SetVRHeadsetParms(projectionMatrix->M,
			framebuffers[0].framebuffers[framebuffers[0].swapchainIndex],
			framebuffers[1].framebuffers[framebuffers[1].swapchainIndex]);

		Com_Frame();

		for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye) {
			engine->framebuffers[eye].swapchainIndex = (engine->framebuffers[eye].swapchainIndex + 1) %
				engine->framebuffers[eye].swapchainLength;
		}

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
			layer.Textures[eye].ColorSwapChain = engine->framebuffers[eye].colorTexture;
			layer.Textures[eye].SwapChainIndex = engine->framebuffers[eye].swapchainIndex;
			layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&defaultProjection);
		}


		const framebuffer_t* framebuffers = engine->framebuffers;

        VR_ClearFrameBuffer(framebuffers[0].framebuffers[framebuffers[0].swapchainIndex], eyeW, eyeH);
        VR_ClearFrameBuffer(framebuffers[1].framebuffers[framebuffers[1].swapchainIndex], eyeW, eyeH);

		re.SetVRHeadsetParms(projectionMatrix->M,
			framebuffers[0].framebuffers[framebuffers[0].swapchainIndex],
			framebuffers[1].framebuffers[framebuffers[1].swapchainIndex]);

		Com_Frame();

		for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye) {
			engine->framebuffers[eye].swapchainIndex = (engine->framebuffers[eye].swapchainIndex + 1) %
				engine->framebuffers[eye].swapchainLength;
		}

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
	}*/

}
