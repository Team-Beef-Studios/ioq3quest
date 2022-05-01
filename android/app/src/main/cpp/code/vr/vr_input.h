#ifndef __VR_INPUT_H
#define __VR_INPUT_H

#if __ANDROID__

void IN_VRInputFrame( void );
void IN_VRInit( void );

void QuatToYawPitchRoll(XrQuaternionf q, vec3_t rotation, vec3_t out);

#endif

#endif
