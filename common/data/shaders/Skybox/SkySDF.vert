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

layout( binding = 1 ) uniform uboTime {
    vec4 times;
} time;

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

layout( location = 0 ) out vec4 modelPos;
layout( location = 1 ) out vec4 timeTime;
layout( location = 2 ) out vec3 camPos;

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
    modelPos = vec4( inPosition, 1.0 );
    timeTime = time.times;

    // Extract the camera position from the view matrix
    camPos = ExtractPos( camera.view );

    // Project coordinate to screen
    vec4 viewPos = camera.view * vec4( inPosition, 0.0 );
    gl_Position = camera.proj * vec4( viewPos.xyz, 1.0 );
}