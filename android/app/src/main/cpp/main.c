#include <jni.h>
#include <memory.h>
#include <string.h>

#include <android/log.h>

#include <VrApi.h>
#include <VrApi_Helpers.h>

#include <client/keycodes.h>
#include <qcommon/q_shared.h>
#include <qcommon/qcommon.h>
#include <vr/vr_base.h>
#include <vr/vr_renderer.h>
#include <unistd.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <pthread.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )

extern void CON_LogcatFn( void (*LogcatFn)( const char* message ) );


#define ALOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "Quake3", __VA_ARGS__))
#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Quake3", __VA_ARGS__))
#define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "Quake3", __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Quake3", __VA_ARGS__))
#define ALOGF(...) ((void)__android_log_print(ANDROID_LOG_FATAL, "Quake3", __VA_ARGS__))

static JNIEnv* g_Env = NULL;
static JavaVM* g_JavaVM = NULL;
static jobject g_ActivityObject = NULL;
static bool    g_HasFocus = true;

void * AppThreadFunction(void * parm );

/*
================================================================================

ovrMessageQueue

================================================================================
*/

typedef enum
{
    MQ_WAIT_NONE,		// don't wait
    MQ_WAIT_RECEIVED,	// wait until the consumer thread has received the message
    MQ_WAIT_PROCESSED	// wait until the consumer thread has processed the message
} ovrMQWait;

#define MAX_MESSAGE_PARMS	8
#define MAX_MESSAGES		1024

typedef struct
{
    int			Id;
    ovrMQWait	Wait;
    long long	Parms[MAX_MESSAGE_PARMS];
} ovrMessage;

static void ovrMessage_Init( ovrMessage * message, const int id, const int wait )
{
    message->Id = id;
    message->Wait = wait;
    memset( message->Parms, 0, sizeof( message->Parms ) );
}

static void		ovrMessage_SetPointerParm( ovrMessage * message, int index, void * ptr ) { *(void **)&message->Parms[index] = ptr; }
static void *	ovrMessage_GetPointerParm( ovrMessage * message, int index ) { return *(void **)&message->Parms[index]; }
static void		ovrMessage_SetIntegerParm( ovrMessage * message, int index, int value ) { message->Parms[index] = value; }
static int		ovrMessage_GetIntegerParm( ovrMessage * message, int index ) { return (int)message->Parms[index]; }
static void		ovrMessage_SetFloatParm( ovrMessage * message, int index, float value ) { *(float *)&message->Parms[index] = value; }
static float	ovrMessage_GetFloatParm( ovrMessage * message, int index ) { return *(float *)&message->Parms[index]; }

// Cyclic queue with messages.
typedef struct
{
    ovrMessage	 		Messages[MAX_MESSAGES];
    volatile int		Head;	// dequeue at the head
    volatile int		Tail;	// enqueue at the tail
    ovrMQWait			Wait;
    volatile bool		EnabledFlag;
    volatile bool		PostedFlag;
    volatile bool		ReceivedFlag;
    volatile bool		ProcessedFlag;
    pthread_mutex_t		Mutex;
    pthread_cond_t		PostedCondition;
    pthread_cond_t		ReceivedCondition;
    pthread_cond_t		ProcessedCondition;
} ovrMessageQueue;

static void ovrMessageQueue_Create( ovrMessageQueue * messageQueue )
{
    messageQueue->Head = 0;
    messageQueue->Tail = 0;
    messageQueue->Wait = MQ_WAIT_NONE;
    messageQueue->EnabledFlag = false;
    messageQueue->PostedFlag = false;
    messageQueue->ReceivedFlag = false;
    messageQueue->ProcessedFlag = false;

    pthread_mutexattr_t	attr;
    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
    pthread_mutex_init( &messageQueue->Mutex, &attr );
    pthread_mutexattr_destroy( &attr );
    pthread_cond_init( &messageQueue->PostedCondition, NULL );
    pthread_cond_init( &messageQueue->ReceivedCondition, NULL );
    pthread_cond_init( &messageQueue->ProcessedCondition, NULL );
}

static void ovrMessageQueue_Destroy( ovrMessageQueue * messageQueue )
{
    pthread_mutex_destroy( &messageQueue->Mutex );
    pthread_cond_destroy( &messageQueue->PostedCondition );
    pthread_cond_destroy( &messageQueue->ReceivedCondition );
    pthread_cond_destroy( &messageQueue->ProcessedCondition );
}

static void ovrMessageQueue_Enable( ovrMessageQueue * messageQueue, const bool set )
{
    messageQueue->EnabledFlag = set;
}

static void ovrMessageQueue_PostMessage( ovrMessageQueue * messageQueue, const ovrMessage * message )
{
    if ( !messageQueue->EnabledFlag )
    {
        return;
    }
    while ( messageQueue->Tail - messageQueue->Head >= MAX_MESSAGES )
    {
        usleep( 1000 );
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    messageQueue->Messages[messageQueue->Tail & ( MAX_MESSAGES - 1 )] = *message;
    messageQueue->Tail++;
    messageQueue->PostedFlag = true;
    pthread_cond_broadcast( &messageQueue->PostedCondition );
    if ( message->Wait == MQ_WAIT_RECEIVED )
    {
        while ( !messageQueue->ReceivedFlag )
        {
            pthread_cond_wait( &messageQueue->ReceivedCondition, &messageQueue->Mutex );
        }
        messageQueue->ReceivedFlag = false;
    }
    else if ( message->Wait == MQ_WAIT_PROCESSED )
    {
        while ( !messageQueue->ProcessedFlag )
        {
            pthread_cond_wait( &messageQueue->ProcessedCondition, &messageQueue->Mutex );
        }
        messageQueue->ProcessedFlag = false;
    }
    pthread_mutex_unlock( &messageQueue->Mutex );
}

static void ovrMessageQueue_SleepUntilMessage( ovrMessageQueue * messageQueue )
{
    if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast( &messageQueue->ProcessedCondition );
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    if ( messageQueue->Tail > messageQueue->Head )
    {
        pthread_mutex_unlock( &messageQueue->Mutex );
        return;
    }
    while ( !messageQueue->PostedFlag )
    {
        pthread_cond_wait( &messageQueue->PostedCondition, &messageQueue->Mutex );
    }
    messageQueue->PostedFlag = false;
    pthread_mutex_unlock( &messageQueue->Mutex );
}

static bool ovrMessageQueue_GetNextMessage( ovrMessageQueue * messageQueue, ovrMessage * message, bool waitForMessages )
{
    if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast( &messageQueue->ProcessedCondition );
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    if ( waitForMessages )
    {
        ovrMessageQueue_SleepUntilMessage( messageQueue );
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    if ( messageQueue->Tail <= messageQueue->Head )
    {
        pthread_mutex_unlock( &messageQueue->Mutex );
        return false;
    }
    *message = messageQueue->Messages[messageQueue->Head & ( MAX_MESSAGES - 1 )];
    messageQueue->Head++;
    pthread_mutex_unlock( &messageQueue->Mutex );
    if ( message->Wait == MQ_WAIT_RECEIVED )
    {
        messageQueue->ReceivedFlag = true;
        pthread_cond_broadcast( &messageQueue->ReceivedCondition );
    }
    else if ( message->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->Wait = MQ_WAIT_PROCESSED;
    }
    return true;
}

/*
================================================================================

ovrAppThread

================================================================================
*/

enum
{
    MESSAGE_ON_CREATE,
    MESSAGE_ON_START,
    MESSAGE_ON_RESUME,
    MESSAGE_ON_PAUSE,
    MESSAGE_ON_STOP,
    MESSAGE_ON_DESTROY,
    MESSAGE_ON_SURFACE_CREATED,
    MESSAGE_ON_SURFACE_DESTROYED
};

typedef struct
{
    pthread_t		Thread;
    ovrMessageQueue	MessageQueue;
    ANativeWindow * NativeWindow;
} ovrAppThread;

static ovrAppThread * gAppThread = NULL;

static void ovrAppThread_Create( ovrAppThread * appThread)
{
    appThread->Thread = 0;
    appThread->NativeWindow = NULL;
    ovrMessageQueue_Create( &appThread->MessageQueue );

    const int createErr = pthread_create( &appThread->Thread, NULL, AppThreadFunction, appThread );
    if ( createErr != 0 )
    {
        ALOGE( "pthread_create returned %i", createErr );
    }
}

static void ovrAppThread_Destroy( ovrAppThread * appThread, JNIEnv * env )
{
    pthread_join( appThread->Thread, NULL );
    ovrMessageQueue_Destroy( &appThread->MessageQueue );
}

JNIEXPORT jlong JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeCreate(JNIEnv* env, jclass cls, jobject thisObject)
{
    g_ActivityObject = (*env)->NewGlobalRef(env, thisObject);

    ovrAppThread * appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
    ovrAppThread_Create( appThread );

    ovrMessageQueue_Enable( &appThread->MessageQueue, true );
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );

    return (jlong)((size_t)appThread);
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeFocusChanged(JNIEnv *env, jclass clazz, jboolean focus)
{
    g_HasFocus = focus;
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeStart( JNIEnv * env, jobject obj, jlong handle, jobject obj1)
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_START, MQ_WAIT_PROCESSED );

    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeResume( JNIEnv * env, jobject obj, jlong handle )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED );

    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativePause( JNIEnv * env, jobject obj, jlong handle )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED );

    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeStop( JNIEnv * env, jobject obj, jlong handle )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED );

    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeDestroy( JNIEnv * env, jobject obj, jlong handle )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    ovrMessageQueue_Enable( &appThread->MessageQueue, false );

    ovrAppThread_Destroy( appThread, env );

    free( appThread );
}

/*
================================================================================

Surface lifecycle

================================================================================
*/

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeSurfaceCreated( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

    ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );

    appThread->NativeWindow = newNativeWindow;
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
    ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeSurfaceChanged( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

    ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );

    if ( newNativeWindow != appThread->NativeWindow )
    {
        if ( appThread->NativeWindow != NULL )
        {
            ovrMessage message;
            ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
            ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
            ANativeWindow_release( appThread->NativeWindow );
            appThread->NativeWindow = NULL;
        }
        if ( newNativeWindow != NULL )
        {
            appThread->NativeWindow = newNativeWindow;
            ovrMessage message;
            ovrMessage_Init( &message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED );
            ovrMessage_SetPointerParm( &message, 0, appThread->NativeWindow );
            ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
        }
    }
    else if ( newNativeWindow != NULL )
    {
        ANativeWindow_release( newNativeWindow );
    }
}

JNIEXPORT void JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeSurfaceDestroyed( JNIEnv * env, jobject obj, jlong handle )
{
    ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
    ovrMessage message;
    ovrMessage_Init( &message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
    ovrMessageQueue_PostMessage( &appThread->MessageQueue, &message );
    ANativeWindow_release( appThread->NativeWindow );
    appThread->NativeWindow = NULL;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    g_JavaVM = vm;
    if ((*g_JavaVM)->GetEnv(g_JavaVM, (void**) &g_Env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }

    return JNI_VERSION_1_4;
}

static void ioq3_logfn(const char* msg)
{
	ALOGI("%s", msg);
}

static ovrJava engine_get_ovrJava() {
	ovrJava java;
	java.Vm = g_JavaVM;
	java.ActivityObject = g_ActivityObject;
	(*java.Vm)->AttachCurrentThread(java.Vm, &java.Env, NULL);
	return java;
}

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

typedef struct
{
    bool multi_view;						// GL_OVR_multiview, GL_OVR_multiview2
    bool EXT_texture_border_clamp;			// GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
} OpenGLExtensions_t;

OpenGLExtensions_t glExtensions;

static void EglInitExtensions()
{
#if defined EGL_SYNC
    eglCreateSyncKHR		= (PFNEGLCREATESYNCKHRPROC)			eglGetProcAddress( "eglCreateSyncKHR" );
	eglDestroySyncKHR		= (PFNEGLDESTROYSYNCKHRPROC)		eglGetProcAddress( "eglDestroySyncKHR" );
	eglClientWaitSyncKHR	= (PFNEGLCLIENTWAITSYNCKHRPROC)		eglGetProcAddress( "eglClientWaitSyncKHR" );
	eglSignalSyncKHR		= (PFNEGLSIGNALSYNCKHRPROC)			eglGetProcAddress( "eglSignalSyncKHR" );
	eglGetSyncAttribKHR		= (PFNEGLGETSYNCATTRIBKHRPROC)		eglGetProcAddress( "eglGetSyncAttribKHR" );
#endif

    const char * allExtensions = (const char *)glGetString( GL_EXTENSIONS );
    if ( allExtensions != NULL )
    {
        glExtensions.multi_view = strstr( allExtensions, "GL_OVR_multiview2" ) &&
                                  strstr( allExtensions, "GL_OVR_multiview_multisampled_render_to_texture" );

        glExtensions.EXT_texture_border_clamp = strstr( allExtensions, "GL_EXT_texture_border_clamp" ) ||
                                                strstr( allExtensions, "GL_OES_texture_border_clamp" );
    }
}

static const char * EglErrorString( const EGLint error )
{
    switch ( error )
    {
        case EGL_SUCCESS:				return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
        default:						return "unknown";
    }
}

static const char * GlFrameBufferStatusString( GLenum status )
{
    switch ( status )
    {
        case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:											return "unknown";
    }
}


/*
================================================================================

ovrEgl

================================================================================
*/

typedef struct
{
    EGLint		MajorVersion;
    EGLint		MinorVersion;
    EGLDisplay	Display;
    EGLConfig	Config;
    EGLSurface	TinySurface;
    EGLSurface	MainSurface;
    EGLContext	Context;
} ovrEgl;

static void ovrEgl_Clear( ovrEgl * egl )
{
    egl->MajorVersion = 0;
    egl->MinorVersion = 0;
    egl->Display = 0;
    egl->Config = 0;
    egl->TinySurface = EGL_NO_SURFACE;
    egl->MainSurface = EGL_NO_SURFACE;
    egl->Context = EGL_NO_CONTEXT;
}

static void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
{
    if ( egl->Display != 0 )
    {
        return;
    }

    egl->Display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    eglInitialize( egl->Display, &egl->MajorVersion, &egl->MinorVersion );
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if ( eglGetConfigs( egl->Display, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE )
    {
        ALOGE( "        eglGetConfigs() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    const EGLint configAttribs[] =
            {
                    EGL_RED_SIZE,		8,
                    EGL_GREEN_SIZE,		8,
                    EGL_BLUE_SIZE,		8,
                    EGL_ALPHA_SIZE,		8, // need alpha for the multi-pass timewarp compositor
                    EGL_DEPTH_SIZE,		24,
                    EGL_STENCIL_SIZE,	8,
                    EGL_SAMPLES,		0,
                    EGL_NONE
            };
    egl->Config = 0;
    for ( int i = 0; i < numConfigs; i++ )
    {
        EGLint value = 0;

        eglGetConfigAttrib( egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value );
        if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
        {
            continue;
        }

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib( egl->Display, configs[i], EGL_SURFACE_TYPE, &value );
        if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
        {
            continue;
        }

        int	j = 0;
        for ( ; configAttribs[j] != EGL_NONE; j += 2 )
        {
            eglGetConfigAttrib( egl->Display, configs[i], configAttribs[j], &value );
            if ( value != configAttribs[j + 1] )
            {
                break;
            }
        }
        if ( configAttribs[j] == EGL_NONE )
        {
            egl->Config = configs[i];
            break;
        }
    }
    if ( egl->Config == 0 )
    {
        ALOGE( "        eglChooseConfig() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    EGLint contextAttribs[] =
            {
                    EGL_CONTEXT_CLIENT_VERSION, 3,
                    EGL_NONE
            };
    ALOGV( "        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )" );
    egl->Context = eglCreateContext( egl->Display, egl->Config, ( shareEgl != NULL ) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs );
    if ( egl->Context == EGL_NO_CONTEXT )
    {
        ALOGE( "        eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    const EGLint surfaceAttribs[] =
            {
                    EGL_WIDTH, 16,
                    EGL_HEIGHT, 16,
                    EGL_NONE
            };
    ALOGV( "        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )" );
    egl->TinySurface = eglCreatePbufferSurface( egl->Display, egl->Config, surfaceAttribs );
    if ( egl->TinySurface == EGL_NO_SURFACE )
    {
        ALOGE( "        eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
    ALOGV( "        eglMakeCurrent( Display, TinySurface, TinySurface, Context )" );
    if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
    {
        ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroySurface( egl->Display, egl->TinySurface );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

static void ovrEgl_DestroyContext( ovrEgl * egl )
{
    if ( egl->Display != 0 )
    {
        ALOGE( "        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )" );
        if ( eglMakeCurrent( egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
        {
            ALOGE( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        }
    }
    if ( egl->Context != EGL_NO_CONTEXT )
    {
        ALOGE( "        eglDestroyContext( Display, Context )" );
        if ( eglDestroyContext( egl->Display, egl->Context ) == EGL_FALSE )
        {
            ALOGE( "        eglDestroyContext() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->Context = EGL_NO_CONTEXT;
    }
    if ( egl->TinySurface != EGL_NO_SURFACE )
    {
        ALOGE( "        eglDestroySurface( Display, TinySurface )" );
        if ( eglDestroySurface( egl->Display, egl->TinySurface ) == EGL_FALSE )
        {
            ALOGE( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->TinySurface = EGL_NO_SURFACE;
    }
    if ( egl->Display != 0 )
    {
        ALOGE( "        eglTerminate( Display )" );
        if ( eglTerminate( egl->Display ) == EGL_FALSE )
        {
            ALOGE( "        eglTerminate() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->Display = 0;
    }
}


/*
================================================================================

ovrApp

================================================================================
*/

#define MAX_TRACKING_SAMPLES 4

typedef struct
{
    ovrJava				Java;
    ovrEgl				Egl;
    ANativeWindow *		NativeWindow;
    bool				Resumed;
    ovrMobile *			Ovr;
    long long			RenderThreadFrameIndex;
    long long			MainThreadFrameIndex;
    double 				DisplayTime[MAX_TRACKING_SAMPLES];
    ovrTracking2        Tracking[MAX_TRACKING_SAMPLES];
    int					SwapInterval;
    int					CpuLevel;
    int					GpuLevel;
    int					MainThreadTid;
    int					RenderThreadTid;
    ovrLayer_Union2		Layers[ovrMaxLayerCount];
    int					LayerCount;
} ovrApp;

static void ovrApp_Clear( ovrApp * app )
{
    app->Java.Vm = NULL;
    app->Java.Env = NULL;
    app->Java.ActivityObject = NULL;
    app->Ovr = NULL;
    app->RenderThreadFrameIndex = 1;
    app->MainThreadFrameIndex = 1;
    memset(app->DisplayTime, 0, MAX_TRACKING_SAMPLES * sizeof(double));
    memset(app->Tracking, 0, MAX_TRACKING_SAMPLES * sizeof(ovrTracking2));
    app->SwapInterval = 1;
    app->CpuLevel = 4;
    app->GpuLevel = 4;
    app->MainThreadTid = 0;
    app->RenderThreadTid = 0;

    ovrEgl_Clear( &app->Egl );
}


static ovrApp gAppState;

bool ioquake3quest_processMessageQueue() {
    for ( ; ; )
    {
        ovrMessage message;
        const bool waitForMessages = ( gAppState.Ovr == NULL );
        if ( !ovrMessageQueue_GetNextMessage( &gAppThread->MessageQueue, &message, waitForMessages ) )
        {
            break;
        }

        switch ( message.Id )
        {
            case MESSAGE_ON_CREATE:
            {
                break;
            }
            case MESSAGE_ON_START:
            {
                break;
            }
            case MESSAGE_ON_RESUME:
            {
                //If we get here, then user has opted not to quit
                gAppState.Resumed = true;
                VR_LeaveVR(VR_GetEngine());
                break;
            }
            case MESSAGE_ON_PAUSE:
            {
                gAppState.Resumed = false;
                VR_EnterVR(VR_GetEngine(), engine_get_ovrJava());
                break;
            }
            case MESSAGE_ON_STOP:
            {
                break;
            }
            case MESSAGE_ON_DESTROY:
            {
                gAppState.NativeWindow = NULL;
                break;
            }
            case MESSAGE_ON_SURFACE_CREATED:	{ gAppState.NativeWindow = (ANativeWindow *)ovrMessage_GetPointerParm( &message, 0 ); break; }
            case MESSAGE_ON_SURFACE_DESTROYED:	{ gAppState.NativeWindow = NULL; break; }
        }
    }

    return true;
}

void * AppThreadFunction(void * parm ) {
    gAppThread = (ovrAppThread *) parm;

    gAppState.CpuLevel = 4;
    gAppState.GpuLevel = 4;
    gAppState.MainThreadTid = gettid();

    sleep(30);

    ovrEgl_CreateContext(&gAppState.Egl, NULL);

    EglInitExtensions();

    ovrJava java = engine_get_ovrJava();
	engine_t* engine = NULL;
	engine = VR_Init(java);

	//First set up resolution cached values
	int width, height;
	VR_GetResolution( engine,  &width, &height );
	
	CON_LogcatFn(&ioq3_logfn);

    char *args = (char*)getenv("commandline");

    Com_Init(args);
    NET_Init( );

	VR_InitRenderer(engine);

	VR_EnterVR(engine, java);

	bool hasFocus = true;
	bool paused = false;
	while (1) {
		if (hasFocus != g_HasFocus) {
			hasFocus = g_HasFocus;
			if (!hasFocus && !Cvar_VariableValue ("cl_paused")) {
				Com_QueueEvent( Sys_Milliseconds(), SE_KEY, K_ESCAPE, qtrue, 0, NULL );
				//Com_QueueEvent( Sys_Milliseconds(), SE_KEY, K_CONSOLE, qtrue, 0, NULL );
				paused = true;
			} else if (hasFocus && paused) {
				//Com_QueueEvent( Sys_Milliseconds(), SE_KEY, K_CONSOLE, qtrue, 0, NULL );
				Com_QueueEvent( Sys_Milliseconds(), SE_KEY, K_ESCAPE, qtrue, 0, NULL );
				paused = false;
			}
		}

		ioquake3quest_processMessageQueue();

		VR_DrawFrame(engine);
	}

	VR_LeaveVR(engine);
	VR_DestroyRenderer(engine);

	Com_Shutdown();
	VR_Destroy(engine);

	return 0;
}
