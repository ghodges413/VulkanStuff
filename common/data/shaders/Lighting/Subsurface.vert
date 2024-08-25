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

layout( location = 0 ) out vec2 fragTexCoord;
layout( location = 1 ) out vec4 worldPos;
layout( location = 2 ) out vec4 worldNormal;
//layout( location = 3 ) out vec4 worldTangent;

//out gl_PerVertex {
//    vec4 gl_Position;
//};

/*
==========================================
main
==========================================
*/
void main() {
    vec4 vert	= vec4( inPosition.xyz, 1.0 );
    
	//  Normal vertex transformation
	gl_Position	= camera.proj * camera.view * model.model * vert;
    fragTexCoord.xy = gl_Position.xy / gl_Position.w;
    fragTexCoord.xy = ( fragTexCoord.xy + 1.0 ) * 0.5;
    
	//  Copy the texture coordinates over
	//fragTexCoord= inTexCoord;

	//  Transform the position to view space
	worldPos	= model.model * vert;
    
	//  Transform the normal and tangent to view space
	vec4 norm	= vec4( inNormal.xyz * 2.0 - 1.0,     0.0 );
	vec4 tang	= vec4( inTangent.xyz * 2.0 - 1.0,    0.0 );
	worldNormal	= model.model * norm;
	worldNormal.xyz = normalize( worldNormal.xyz );
	//worldNormal.xyz = ( worldNormal.xyz + vec3( 1 ) ) * 0.5;
	//worldNormal = vec4( 0, 0, 1, 0 );
	//worldTangent= model.model * tang;
}