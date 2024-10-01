#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
==========================================
Uniforms
==========================================
*/

layout( binding = 0 ) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;
layout( binding = 1 ) uniform uboModel {
    mat4 model;
} model;

/*
==========================================
attributes
==========================================
*/

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec2 inTexCoord;
layout( location = 2 ) in vec4 inNormal;
layout( location = 3 ) in vec4 inTangent;
layout( location = 4 ) in vec4 inColor;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 worldPos;

/*
==========================================
main
==========================================
*/
void main() {
    vec4 vert	= vec4( inPosition.xyz, 1.0 );
    
	//  Normal vertex transformation
	gl_Position	= camera.proj * camera.view * model.model * vert;

	//  Transform the position to view space
	worldPos	= model.model * vert;
}