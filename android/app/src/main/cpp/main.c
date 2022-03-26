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

#include <SDL.h>

extern void CON_LogcatFn( void (*LogcatFn)( const char* message ) );


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
    JavaVM *		JavaVm;
    jobject			ActivityObject;
    jclass          ActivityClass;
    pthread_t		Thread;
    ovrMessageQueue	MessageQueue;
    ANativeWindow * NativeWindow;
} ovrAppThread;


static void ovrAppThread_Create( ovrAppThread * appThread, JNIEnv * env, jobject activityObject, jclass activityClass )
{
    (*env)->GetJavaVM( env, &appThread->JavaVm );
    appThread->ActivityObject = (*env)->NewGlobalRef( env, activityObject );
    appThread->ActivityClass = (*env)->NewGlobalRef( env, activityClass );
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
    (*env)->DeleteGlobalRef( env, appThread->ActivityObject );
    (*env)->DeleteGlobalRef( env, appThread->ActivityClass );
    ovrMessageQueue_Destroy( &appThread->MessageQueue );
}

JNIEXPORT jlong JNICALL Java_com_drbeef_ioq3quest_MainActivity_nativeCreate(JNIEnv* env, jclass cls, jobject thisObject)
{
    g_ActivityObject = (*env)->NewGlobalRef(env, thisObject);

    ovrAppThread * appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
    ovrAppThread_Create( appThread, env, thisObject, cls );

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
	LOGI("%s", msg);
}

static ovrJava engine_get_ovrJava() {
	ovrJava java;
	java.Vm = g_JavaVM;
	java.ActivityObject = g_ActivityObject;
	(*java.Vm)->AttachCurrentThread(java.Vm, &java.Env, NULL);
	return java;
}

void * AppThreadFunction(void * parm ) {
	ovrJava java = engine_get_ovrJava();
	engine_t* engine = NULL;
	engine = VR_Init(java);

	sleep(30);

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

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			LOGI("Received SDL Event: %d", event.type);
			switch (event.type)
			{
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					VR_EnterVR(engine, engine_get_ovrJava());
					break;

				case SDL_WINDOWEVENT_FOCUS_LOST:
					VR_LeaveVR(engine);
					break;
			}
		}

		VR_DrawFrame(engine);
	}

	VR_LeaveVR(engine);
	VR_DestroyRenderer(engine);

	Com_Shutdown();
	VR_Destroy(engine);

	return 0;
}
