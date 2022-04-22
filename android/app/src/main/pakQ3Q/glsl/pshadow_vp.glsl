attribute vec3 attr_Position;
attribute vec3 attr_Normal;

uniform mat4 u_ModelMatrix;

layout(shared) uniform ViewMatrices
{
    uniform mat4 u_ViewMatrices[NUM_VIEWS];
};
layout(shared) uniform ProjectionMatrix
{
    uniform mat4 u_ProjectionMatrix;
};

varying vec3   var_Position;
varying vec3   var_Normal;


void main()
{
	gl_Position = u_ProjectionMatrix * (u_ViewMatrices[gl_ViewID_OVR] * (u_ModelMatrix * vec4(attr_Position, 1.0)));

	var_Position  = attr_Position;
	var_Normal    = attr_Normal;
}
