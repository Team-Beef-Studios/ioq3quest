#ifndef __VR_INPUT_H
#define __VR_INPUT_H

#include "../qcommon/q_shared.h"
#include "vr_types.h"

#if __ANDROID__

void IN_VRInputFrame( void );
void IN_VRInit( void );
void IN_VRSyncActions( void );
XrPosef IN_VRUpdateHMD( float predictedDisplayTime );
void IN_VRUpdateControllers( float predictedDisplayTime );

void QuatToYawPitchRoll(XrQuaternionf q, vec3_t rotation, vec3_t out);

#endif

#endif
