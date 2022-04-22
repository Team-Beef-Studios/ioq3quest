attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;

// Uniforms

uniform mat4 u_ModelMatrix;

layout(shared) uniform ViewMatrices
{
    uniform mat4 u_ViewMatrices[NUM_VIEWS];
};
layout(shared) uniform ProjectionMatrix
{
    uniform mat4 u_ProjectionMatrix;
};

varying vec2   var_Tex1;


void main()
{
	gl_Position = u_ProjectionMatrix * (u_ViewMatrices[gl_ViewID_OVR] * (u_ModelMatrix * vec4(attr_Position, 1.0)));

	var_Tex1 = attr_TexCoord0.st;
}
