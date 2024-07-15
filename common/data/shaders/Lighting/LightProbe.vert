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
layout( location = 1 ) out vec3 cameraPos;

out gl_PerVertex {
    vec4 gl_Position;
};

/*
==========================================
ExtractPos
Gets the camera position from the view matrix
==========================================
*/
vec3 ExtractPos( in mat4 matrix ) {
    mat4 invMatrix = inverse( matrix );
    vec4 pos = invMatrix * vec4( 0, 0, 0, 1 );
    return pos.xyz;
}

/*
==========================================
main
==========================================
*/
void main() {
    // Extract the camera position from the view matrix
    cameraPos = ExtractPos( camera.view );

    // Project coordinate to screen
    gl_Position = vec4( inPosition, 1.0 );
    fragTexCoord = inTexCoord;
    fragTexCoord.y = 1.0 - fragTexCoord.y;
}