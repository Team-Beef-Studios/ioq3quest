
//
// VR Hud Shader Sprite - the image is just a placeholder and is replaced in code
//

sprites/vr/hud
{
	cull disable
	{
		clampmap sprites/plasmaa.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}