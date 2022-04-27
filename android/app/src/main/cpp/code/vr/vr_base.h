#ifndef __VR_BASE
#define __VR_BASE

#if __ANDROID__

#include "vr_types.h"

engine_t* VR_Init( ovrJava java );
void VR_InitCvars( void );
void VR_Destroy( engine_t* engine );
void VR_EnterVR( engine_t* engine, ovrJava java );
void VR_LeaveVR( engine_t* engine );

engine_t* VR_GetEngine( void );
int VR_useScreenLayer( void );
int VR_isPauseable( void );

float radians(float deg);

void VR_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight );

#endif

#endif
