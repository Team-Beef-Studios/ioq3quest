#include "vr_types.h"

/************************************************************************************

Original file name  : XrCompositor_NativeActivity.c

Copyright : Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <assert.h>

const int NUM_MULTI_SAMPLES = 4;

/*
================================================================================

ovrFramebuffer

================================================================================
*/


void ovrFramebuffer_Clear(ovrFramebuffer* frameBuffer) {
    frameBuffer->Width = 0;
    frameBuffer->Height = 0;
    frameBuffer->Multisamples = 0;
    frameBuffer->TextureSwapChainLength = 0;
    frameBuffer->TextureSwapChainIndex = 0;
    frameBuffer->ColorSwapChain.Handle = XR_NULL_HANDLE;
    frameBuffer->ColorSwapChain.Width = 0;
    frameBuffer->ColorSwapChain.Height = 0;
    frameBuffer->ColorSwapChainImage = NULL;
    frameBuffer->DepthBuffers = NULL;
    frameBuffer->FrameBuffers = NULL;
}

bool ovrFramebuffer_Create(
        XrSession session,
        ovrFramebuffer* frameBuffer,
        const GLenum colorFormat,
        const int width,
        const int height,
        const int multisamples) {
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glRenderbufferStorageMultisampleEXT");
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress(
                    "glFramebufferTexture2DMultisampleEXT");

    frameBuffer->Width = width;
    frameBuffer->Height = height;
    frameBuffer->Multisamples = multisamples;

    GLenum requestedGLFormat = colorFormat;

    // Get the number of supported formats.
    uint32_t numInputFormats = 0;
    uint32_t numOutputFormats = 0;
    OXR(xrEnumerateSwapchainFormats(session, numInputFormats, &numOutputFormats, NULL));

    // Allocate an array large enough to contain the supported formats.
    numInputFormats = numOutputFormats;
    int64_t* supportedFormats = (int64_t*)malloc(numOutputFormats * sizeof(int64_t));
    if (supportedFormats != NULL) {
        OXR(xrEnumerateSwapchainFormats(
                session, numInputFormats, &numOutputFormats, supportedFormats));
    }

    // Verify the requested format is supported.
    uint64_t selectedFormat = 0;
    for (uint32_t i = 0; i < numOutputFormats; i++) {
        if (supportedFormats[i] == requestedGLFormat) {
            selectedFormat = supportedFormats[i];
            break;
        }
    }

    free(supportedFormats);

    if (selectedFormat == 0) {
        ALOGE("Format not supported");
    }

    XrSwapchainCreateInfo swapChainCreateInfo;
    memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
    swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapChainCreateInfo.usageFlags =
            XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.format = selectedFormat;
    swapChainCreateInfo.sampleCount = 1;
    swapChainCreateInfo.width = width;
    swapChainCreateInfo.height = height;
    swapChainCreateInfo.faceCount = 1;
    swapChainCreateInfo.arraySize = 1;
    swapChainCreateInfo.mipCount = 1;

    // Enable Foveation on this swapchain
    XrSwapchainCreateInfoFoveationFB swapChainFoveationCreateInfo;
    memset(&swapChainFoveationCreateInfo, 0, sizeof(swapChainFoveationCreateInfo));
    swapChainFoveationCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB;
    swapChainCreateInfo.next = &swapChainFoveationCreateInfo;

    frameBuffer->ColorSwapChain.Width = swapChainCreateInfo.width;
    frameBuffer->ColorSwapChain.Height = swapChainCreateInfo.height;

    // Create the swapchain.
    OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &frameBuffer->ColorSwapChain.Handle));
    // Get the number of swapchain images.
    OXR(xrEnumerateSwapchainImages(
            frameBuffer->ColorSwapChain.Handle, 0, &frameBuffer->TextureSwapChainLength, NULL));
    // Allocate the swapchain images array.
    frameBuffer->ColorSwapChainImage = (XrSwapchainImageOpenGLESKHR*)malloc(
            frameBuffer->TextureSwapChainLength * sizeof(XrSwapchainImageOpenGLESKHR));

    // Populate the swapchain image array.
    for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
        frameBuffer->ColorSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        frameBuffer->ColorSwapChainImage[i].next = NULL;
    }
    OXR(xrEnumerateSwapchainImages(
            frameBuffer->ColorSwapChain.Handle,
            frameBuffer->TextureSwapChainLength,
            &frameBuffer->TextureSwapChainLength,
            (XrSwapchainImageBaseHeader*)frameBuffer->ColorSwapChainImage));

    frameBuffer->DepthBuffers =
            (GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));
    frameBuffer->FrameBuffers =
            (GLuint*)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));

    for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
        // Create the color buffer texture.
        const GLuint colorTexture = frameBuffer->ColorSwapChainImage[i].image;

        GLenum colorTextureTarget = GL_TEXTURE_2D;
        GL(glBindTexture(colorTextureTarget, colorTexture));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glBindTexture(colorTextureTarget, 0));

        if (multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL &&
            glFramebufferTexture2DMultisampleEXT != NULL) {
            // Create multisampled depth buffer.
            GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
            GL(glRenderbufferStorageMultisampleEXT(
                    GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            // NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
            GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
            GL(glFramebufferTexture2DMultisampleEXT(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D,
                    colorTexture,
                    0,
                    multisamples));
            GL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER,
                    frameBuffer->DepthBuffers[i]));
            GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER));
            GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                ALOGE(
                        "Incomplete frame buffer object: %d", renderFramebufferStatus);
                return false;
            }
        } else {
            // Create depth buffer.
            GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
            GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
            GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Create the frame buffer.
            GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
            GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
            GL(glFramebufferRenderbuffer(
                    GL_DRAW_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER,
                    frameBuffer->DepthBuffers[i]));
            GL(glFramebufferTexture2D(
                    GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
            GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
            GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
            if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                ALOGE(
                        "Incomplete frame buffer object: %d", renderFramebufferStatus);
                return false;
            }
        }
    }

    return true;
}

void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer) {
    GL(glDeleteFramebuffers(frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers));
    GL(glDeleteRenderbuffers(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
    OXR(xrDestroySwapchain(frameBuffer->ColorSwapChain.Handle));
    free(frameBuffer->ColorSwapChainImage);

    free(frameBuffer->DepthBuffers);
    free(frameBuffer->FrameBuffers);

    ovrFramebuffer_Clear(frameBuffer);
}

void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer) {
    GL(glBindFramebuffer(
            GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex]));
}

void ovrFramebuffer_SetNone() {
    GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer) {
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
    glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
}

void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer) {
    // Acquire the swapchain image
    XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
    OXR(xrAcquireSwapchainImage(
            frameBuffer->ColorSwapChain.Handle, &acquireInfo, &frameBuffer->TextureSwapChainIndex));

    XrSwapchainImageWaitInfo waitInfo;
    waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    waitInfo.next = NULL;
    waitInfo.timeout = 1000000000; /* timeout in nanoseconds */
    XrResult res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
    int i = 0;
    while (res == XR_TIMEOUT_EXPIRED) {
        res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
        i++;
        ALOGV(
                " Retry xrWaitSwapchainImage %d times due to XR_TIMEOUT_EXPIRED (duration %f seconds)",
                i,
                waitInfo.timeout * (1E-9));
    }
}

void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer) {
    XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
    OXR(xrReleaseSwapchainImage(frameBuffer->ColorSwapChain.Handle, &releaseInfo));
}

/*
================================================================================

ovrRenderer

================================================================================
*/

void ovrRenderer_Clear(ovrRenderer* renderer) {
    for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
        ovrFramebuffer_Clear(&renderer->FrameBuffer[eye]);
    }
}

void ovrRenderer_Create(
        XrSession session,
        ovrRenderer* renderer,
        int suggestedEyeTextureWidth,
        int suggestedEyeTextureHeight) {
    // Create the frame buffers.
    for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
        ovrFramebuffer_Create(
                session,
                &renderer->FrameBuffer[eye],
                GL_SRGB8_ALPHA8,
                suggestedEyeTextureWidth,
                suggestedEyeTextureHeight,
                NUM_MULTI_SAMPLES);
    }
}

void ovrRenderer_Destroy(ovrRenderer* renderer) {
    for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
        ovrFramebuffer_Destroy(&renderer->FrameBuffer[eye]);
    }
}

/*
================================================================================

ovrApp

================================================================================
*/

void ovrApp_Clear(ovrApp* app) {
    app->Focused = false;
    app->Instance = XR_NULL_HANDLE;
    app->Session = XR_NULL_HANDLE;
    memset(&app->ViewportConfig, 0, sizeof(XrViewConfigurationProperties));
    memset(&app->ViewConfigurationView, 0, ovrMaxNumEyes * sizeof(XrViewConfigurationView));
    app->SystemId = XR_NULL_SYSTEM_ID;
    app->HeadSpace = XR_NULL_HANDLE;
    app->LocalSpace = XR_NULL_HANDLE;
    app->StageSpace = XR_NULL_HANDLE;
    app->FakeStageSpace = XR_NULL_HANDLE;
    app->CurrentSpace = XR_NULL_HANDLE;
    app->SessionActive = false;
    app->SupportedDisplayRefreshRates = NULL;
    app->RequestedDisplayRefreshRateIndex = 0;
    app->NumSupportedDisplayRefreshRates = 0;
    app->pfnGetDisplayRefreshRate = NULL;
    app->pfnRequestDisplayRefreshRate = NULL;
    app->SwapInterval = 1;
    memset(app->Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);
    app->LayerCount = 0;
    app->MainThreadTid = 0;
    app->RenderThreadTid = 0;
    app->TouchPadDownLastFrame = false;

    ovrRenderer_Clear(&app->Renderer);
}

void ovrApp_Destroy(ovrApp* app) {
    if (app->SupportedDisplayRefreshRates != NULL) {
        free(app->SupportedDisplayRefreshRates);
    }

    ovrApp_Clear(app);
}

void ovrApp_HandleSessionStateChanges(ovrApp* app, XrSessionState state) {
    if (state == XR_SESSION_STATE_READY) {
        assert(app->SessionActive == false);

        XrSessionBeginInfo sessionBeginInfo;
        memset(&sessionBeginInfo, 0, sizeof(sessionBeginInfo));
        sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
        sessionBeginInfo.next = NULL;
        sessionBeginInfo.primaryViewConfigurationType = app->ViewportConfig.viewConfigurationType;

        XrResult result;
        OXR(result = xrBeginSession(app->Session, &sessionBeginInfo));

        app->SessionActive = (result == XR_SUCCESS);

        // Set session state once we have entered VR mode and have a valid session object.
        if (app->SessionActive) {
            XrPerfSettingsLevelEXT cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
            XrPerfSettingsLevelEXT gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;

            PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
            OXR(xrGetInstanceProcAddr(
                    app->Instance,
                    "xrPerfSettingsSetPerformanceLevelEXT",
                    (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT)));

            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                    app->Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, cpuPerfLevel));
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                    app->Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, gpuPerfLevel));

            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(
                    app->Instance,
                    "xrSetAndroidApplicationThreadKHR",
                    (PFN_xrVoidFunction*)(&pfnSetAndroidApplicationThreadKHR)));

            OXR(pfnSetAndroidApplicationThreadKHR(
                    app->Session, XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, app->MainThreadTid));
            OXR(pfnSetAndroidApplicationThreadKHR(
                    app->Session, XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, app->RenderThreadTid));
        }
    } else if (state == XR_SESSION_STATE_STOPPING) {
        assert(app->SessionActive);

        OXR(xrEndSession(app->Session));
        app->SessionActive = false;
    }
}

void ovrApp_HandleXrEvents(ovrApp* app) {
    XrEventDataBuffer eventDataBuffer = {};

    // Poll for events
    for (;;) {
        XrEventDataBaseHeader* baseEventHeader = (XrEventDataBaseHeader*)(&eventDataBuffer);
        baseEventHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
        baseEventHeader->next = NULL;
        XrResult r;
        OXR(r = xrPollEvent(app->Instance, &eventDataBuffer));
        if (r != XR_SUCCESS) {
            break;
        }

        switch (baseEventHeader->type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_EVENTS_LOST event");
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                const XrEventDataInstanceLossPending* instance_loss_pending_event =
                        (XrEventDataInstanceLossPending*)(baseEventHeader);
                ALOGV(
                        "xrPollEvent: received XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING event: time %f",
                        FromXrTime(instance_loss_pending_event->lossTime));
            } break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event");
                break;
            case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
                const XrEventDataPerfSettingsEXT* perf_settings_event =
                        (XrEventDataPerfSettingsEXT*)(baseEventHeader);
                ALOGV(
                        "xrPollEvent: received XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT event: type %d subdomain %d : level %d -> level %d",
                        perf_settings_event->type,
                        perf_settings_event->subDomain,
                        perf_settings_event->fromLevel,
                        perf_settings_event->toLevel);
            } break;
            case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB: {
                const XrEventDataDisplayRefreshRateChangedFB* refresh_rate_changed_event =
                        (XrEventDataDisplayRefreshRateChangedFB*)(baseEventHeader);
                ALOGV(
                        "xrPollEvent: received XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB event: fromRate %f -> toRate %f",
                        refresh_rate_changed_event->fromDisplayRefreshRate,
                        refresh_rate_changed_event->toDisplayRefreshRate);
            } break;
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                XrEventDataReferenceSpaceChangePending* ref_space_change_event =
                        (XrEventDataReferenceSpaceChangePending*)(baseEventHeader);
                ALOGV(
                        "xrPollEvent: received XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING event: changed space: %d for session %p at time %f",
                        ref_space_change_event->referenceSpaceType,
                        (void*)ref_space_change_event->session,
                        FromXrTime(ref_space_change_event->changeTime));
            } break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const XrEventDataSessionStateChanged* session_state_changed_event =
                        (XrEventDataSessionStateChanged*)(baseEventHeader);
                ALOGV(
                        "xrPollEvent: received XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: %d for session %p at time %f",
                        session_state_changed_event->state,
                        (void*)session_state_changed_event->session,
                        FromXrTime(session_state_changed_event->time));

                switch (session_state_changed_event->state) {
                    case XR_SESSION_STATE_FOCUSED:
                        app->Focused = true;
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        app->Focused = false;
                        break;
                    case XR_SESSION_STATE_READY:
                    case XR_SESSION_STATE_STOPPING:
                        ovrApp_HandleSessionStateChanges(app, session_state_changed_event->state);
                        break;
                    default:
                        break;
                }
            } break;
            default:
                ALOGV("xrPollEvent: Unknown event");
                break;
        }
    }
}

/*
================================================================================

ovrMatrix4f

================================================================================
*/


ovrMatrix4f ovrMatrix4f_CreateProjection(
        const float minX,
        const float maxX,
        float const minY,
        const float maxY,
        const float nearZ,
        const float farZ) {
    const float width = maxX - minX;
    const float height = maxY - minY;
    const float offsetZ = nearZ; // set to zero for a [0,1] clip space

    ovrMatrix4f out;
    if (farZ <= nearZ) {
        // place the far plane at infinity
        out.M[0][0] = 2 * nearZ / width;
        out.M[0][1] = 0;
        out.M[0][2] = (maxX + minX) / width;
        out.M[0][3] = 0;

        out.M[1][0] = 0;
        out.M[1][1] = 2 * nearZ / height;
        out.M[1][2] = (maxY + minY) / height;
        out.M[1][3] = 0;

        out.M[2][0] = 0;
        out.M[2][1] = 0;
        out.M[2][2] = -1;
        out.M[2][3] = -(nearZ + offsetZ);

        out.M[3][0] = 0;
        out.M[3][1] = 0;
        out.M[3][2] = -1;
        out.M[3][3] = 0;
    } else {
        // normal projection
        out.M[0][0] = 2 * nearZ / width;
        out.M[0][1] = 0;
        out.M[0][2] = (maxX + minX) / width;
        out.M[0][3] = 0;

        out.M[1][0] = 0;
        out.M[1][1] = 2 * nearZ / height;
        out.M[1][2] = (maxY + minY) / height;
        out.M[1][3] = 0;

        out.M[2][0] = 0;
        out.M[2][1] = 0;
        out.M[2][2] = -(farZ + offsetZ) / (farZ - nearZ);
        out.M[2][3] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

        out.M[3][0] = 0;
        out.M[3][1] = 0;
        out.M[3][2] = -1;
        out.M[3][3] = 0;
    }
    return out;
}

ovrMatrix4f ovrMatrix4f_CreateProjectionFov(
        const float fovDegreesX,
        const float fovDegreesY,
        const float offsetX,
        const float offsetY,
        const float nearZ,
        const float farZ) {
    const float halfWidth = nearZ * tanf(fovDegreesX * (M_PI / 180.0f * 0.5f));
    const float halfHeight = nearZ * tanf(fovDegreesY * (M_PI / 180.0f * 0.5f));

    const float minX = offsetX - halfWidth;
    const float maxX = offsetX + halfWidth;

    const float minY = offsetY - halfHeight;
    const float maxY = offsetY + halfHeight;

    return ovrMatrix4f_CreateProjection(minX, maxX, minY, maxY, nearZ, farZ);
}

ovrMatrix4f ovrMatrix4f_CreateFromQuaternion(const XrQuaternionf* q) {
    const float ww = q->w * q->w;
    const float xx = q->x * q->x;
    const float yy = q->y * q->y;
    const float zz = q->z * q->z;

    ovrMatrix4f out;
    out.M[0][0] = ww + xx - yy - zz;
    out.M[0][1] = 2 * (q->x * q->y - q->w * q->z);
    out.M[0][2] = 2 * (q->x * q->z + q->w * q->y);
    out.M[0][3] = 0;

    out.M[1][0] = 2 * (q->x * q->y + q->w * q->z);
    out.M[1][1] = ww - xx + yy - zz;
    out.M[1][2] = 2 * (q->y * q->z - q->w * q->x);
    out.M[1][3] = 0;

    out.M[2][0] = 2 * (q->x * q->z - q->w * q->y);
    out.M[2][1] = 2 * (q->y * q->z + q->w * q->x);
    out.M[2][2] = ww - xx - yy + zz;
    out.M[2][3] = 0;

    out.M[3][0] = 0;
    out.M[3][1] = 0;
    out.M[3][2] = 0;
    out.M[3][3] = 1;
    return out;
}


/// Use left-multiplication to accumulate transformations.
ovrMatrix4f ovrMatrix4f_Multiply(const ovrMatrix4f* a, const ovrMatrix4f* b) {
    ovrMatrix4f out;
    out.M[0][0] = a->M[0][0] * b->M[0][0] + a->M[0][1] * b->M[1][0] + a->M[0][2] * b->M[2][0] +
                  a->M[0][3] * b->M[3][0];
    out.M[1][0] = a->M[1][0] * b->M[0][0] + a->M[1][1] * b->M[1][0] + a->M[1][2] * b->M[2][0] +
                  a->M[1][3] * b->M[3][0];
    out.M[2][0] = a->M[2][0] * b->M[0][0] + a->M[2][1] * b->M[1][0] + a->M[2][2] * b->M[2][0] +
                  a->M[2][3] * b->M[3][0];
    out.M[3][0] = a->M[3][0] * b->M[0][0] + a->M[3][1] * b->M[1][0] + a->M[3][2] * b->M[2][0] +
                  a->M[3][3] * b->M[3][0];

    out.M[0][1] = a->M[0][0] * b->M[0][1] + a->M[0][1] * b->M[1][1] + a->M[0][2] * b->M[2][1] +
                  a->M[0][3] * b->M[3][1];
    out.M[1][1] = a->M[1][0] * b->M[0][1] + a->M[1][1] * b->M[1][1] + a->M[1][2] * b->M[2][1] +
                  a->M[1][3] * b->M[3][1];
    out.M[2][1] = a->M[2][0] * b->M[0][1] + a->M[2][1] * b->M[1][1] + a->M[2][2] * b->M[2][1] +
                  a->M[2][3] * b->M[3][1];
    out.M[3][1] = a->M[3][0] * b->M[0][1] + a->M[3][1] * b->M[1][1] + a->M[3][2] * b->M[2][1] +
                  a->M[3][3] * b->M[3][1];

    out.M[0][2] = a->M[0][0] * b->M[0][2] + a->M[0][1] * b->M[1][2] + a->M[0][2] * b->M[2][2] +
                  a->M[0][3] * b->M[3][2];
    out.M[1][2] = a->M[1][0] * b->M[0][2] + a->M[1][1] * b->M[1][2] + a->M[1][2] * b->M[2][2] +
                  a->M[1][3] * b->M[3][2];
    out.M[2][2] = a->M[2][0] * b->M[0][2] + a->M[2][1] * b->M[1][2] + a->M[2][2] * b->M[2][2] +
                  a->M[2][3] * b->M[3][2];
    out.M[3][2] = a->M[3][0] * b->M[0][2] + a->M[3][1] * b->M[1][2] + a->M[3][2] * b->M[2][2] +
                  a->M[3][3] * b->M[3][2];

    out.M[0][3] = a->M[0][0] * b->M[0][3] + a->M[0][1] * b->M[1][3] + a->M[0][2] * b->M[2][3] +
                  a->M[0][3] * b->M[3][3];
    out.M[1][3] = a->M[1][0] * b->M[0][3] + a->M[1][1] * b->M[1][3] + a->M[1][2] * b->M[2][3] +
                  a->M[1][3] * b->M[3][3];
    out.M[2][3] = a->M[2][0] * b->M[0][3] + a->M[2][1] * b->M[1][3] + a->M[2][2] * b->M[2][3] +
                  a->M[2][3] * b->M[3][3];
    out.M[3][3] = a->M[3][0] * b->M[0][3] + a->M[3][1] * b->M[1][3] + a->M[3][2] * b->M[2][3] +
                  a->M[3][3] * b->M[3][3];
    return out;
}

ovrMatrix4f ovrMatrix4f_CreateRotation(const float radiansX, const float radiansY, const float radiansZ) {
    const float sinX = sinf(radiansX);
    const float cosX = cosf(radiansX);
    const ovrMatrix4f rotationX = {
            {{1, 0, 0, 0}, {0, cosX, -sinX, 0}, {0, sinX, cosX, 0}, {0, 0, 0, 1}}};
    const float sinY = sinf(radiansY);
    const float cosY = cosf(radiansY);
    const ovrMatrix4f rotationY = {
            {{cosY, 0, sinY, 0}, {0, 1, 0, 0}, {-sinY, 0, cosY, 0}, {0, 0, 0, 1}}};
    const float sinZ = sinf(radiansZ);
    const float cosZ = cosf(radiansZ);
    const ovrMatrix4f rotationZ = {
            {{cosZ, -sinZ, 0, 0}, {sinZ, cosZ, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
    const ovrMatrix4f rotationXY = ovrMatrix4f_Multiply(&rotationY, &rotationX);
    return ovrMatrix4f_Multiply(&rotationZ, &rotationXY);
}

XrVector4f XrVector4f_MultiplyMatrix4f(const ovrMatrix4f* a, const XrVector4f* v) {
    XrVector4f out;
    out.x = a->M[0][0] * v->x + a->M[0][1] * v->y + a->M[0][2] * v->z + a->M[0][3] * v->w;
    out.y = a->M[1][0] * v->x + a->M[1][1] * v->y + a->M[1][2] * v->z + a->M[1][3] * v->w;
    out.z = a->M[2][0] * v->x + a->M[2][1] * v->y + a->M[2][2] * v->z + a->M[2][3] * v->w;
    out.w = a->M[3][0] * v->x + a->M[3][1] * v->y + a->M[3][2] * v->z + a->M[3][3] * v->w;
    return out;
}