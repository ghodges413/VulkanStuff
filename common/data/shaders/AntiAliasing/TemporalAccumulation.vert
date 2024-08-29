#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
==========================================
Uniforms
==========================================
*/

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

layout( location = 0 ) out vec2 fragTexCoord;

/*
==========================================
main
==========================================
*/
void main() {
    vec4 vert	= vec4( inPosition.xyz, 1.0 );
	fragTexCoord = ( vert.xy + 1.0 ) * 0.5;
    
	gl_Position	= vert;
}