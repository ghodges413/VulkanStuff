#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform pushCamera {
    mat4 cameraNew;
    mat4 cameraOld;
} PushConstant;
layout( binding = 0 ) uniform uboModel {
    mat4 model;
} model;
layout( binding = 1 ) uniform uboModelOld {
    mat4 model;
} modelOld;

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

layout( location = 0 ) out vec4 fragTexCoord;
layout( location = 1 ) out vec4 fragTexCoordOld;

/*
==========================================
main
==========================================
*/
void main() {
    vec4 vert	= vec4( inPosition.xyz, 1.0 );
    
	gl_Position	= PushConstant.cameraNew * model.model * vert;
	fragTexCoord = gl_Position;

	vec4 oldPosition = PushConstant.cameraOld * modelOld.model * vert;
	fragTexCoordOld = oldPosition;
}