attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;


uniform mat4 u_ModelMatrix;
// Uniforms
layout(shared) uniform ViewMatrices
{
    uniform mat4 u_ViewMatrices[NUM_VIEWS];
};
layout(shared) uniform ProjectionMatrix
{
    uniform mat4 u_ProjectionMatrix;
};

varying vec2   var_TexCoords;


void main()
{
	gl_Position = u_ProjectionMatrix * (u_ViewMatrices[gl_ViewID_OVR] * (u_ModelMatrix * vec4(attr_Position, 1.0)));

	var_TexCoords = attr_TexCoord0.st;
}
