attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;

// Uniforms
uniform mat4 u_ModelMatrix;

uniform vec3   u_ToneMinAvgMaxLinear;

layout(shared) uniform ViewMatrices
{
    uniform mat4 u_ViewMatrices[NUM_VIEWS];
};
layout(shared) uniform ProjectionMatrix
{
    uniform mat4 u_ProjectionMatrix;
};


varying vec2   var_TexCoords;
varying float  var_InvWhite;

float FilmicTonemap(float x)
{
	const float SS  = 0.22; // Shoulder Strength
	const float LS  = 0.30; // Linear Strength
	const float LA  = 0.10; // Linear Angle
	const float TS  = 0.20; // Toe Strength
	const float TAN = 0.01; // Toe Angle Numerator
	const float TAD = 0.30; // Toe Angle Denominator

	return ((x*(SS*x+LA*LS)+TS*TAN)/(x*(SS*x+LS)+TS*TAD)) - TAN/TAD;
}

void main()
{
	gl_Position = u_ProjectionMatrix * (u_ViewMatrices[gl_ViewID_OVR] * (u_ModelMatrix * vec4(attr_Position, 1.0)));
	var_TexCoords = attr_TexCoord0.st;
	var_InvWhite = 1.0 / FilmicTonemap(u_ToneMinAvgMaxLinear.z - u_ToneMinAvgMaxLinear.x);
}
