#ifndef __VR_RENDERER
#define __VR_RENDERER

#if __ANDROID__

#include "vr_types.h"

void VR_GetResolution( engine_t* engine, int *pWidth, int *pHeight );
void VR_InitRenderer( engine_t* engine );
void VR_DestroyRenderer( engine_t* engine );
void VR_DrawFrame( engine_t* engine );
void VR_ReInitRenderer();

#endif

#endif
