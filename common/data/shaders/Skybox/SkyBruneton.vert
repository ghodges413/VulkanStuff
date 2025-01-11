#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
==========================================
uniforms
==========================================
*/

layout( binding = 0 ) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;

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
layout( location = 1 ) out vec3 ray;
layout( location = 2 ) out mat4 v_viewInverse;

void main() {
	vec4 vertex = vec4( inPosition.xyz, 1.0f );

    mat4 viewInverse = inverse( camera.view );
    mat4 projInverse = inverse( camera.proj );
	
    //coords = vertex.xy * 0.5 + 0.5;
    fragTexCoord = vertex.xy * 0.5 + 0.5;

    ray = ( viewInverse * vec4( ( projInverse * vertex ).xyz, 0.0 ) ).xyz;
    gl_Position = vertex;
	v_viewInverse = viewInverse;
}
