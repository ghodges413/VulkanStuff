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
layout( location = 1 ) out vec3 cameraPos;
layout( location = 2 ) out vec4 worldPos;
layout( location = 3 ) out vec4 worldNormal;
layout( location = 4 ) out vec4 worldTangent;

out gl_PerVertex {
    vec4 gl_Position;
};

vec3 ExtractPos( in mat4 matrix ) {
    vec3 pos;
    pos = -matrix[ 3 ].xyz;
    //pos.x = -matrix[ 0 ].w;
    //pos.y = -matrix[ 1 ].w;
    //pos.z = -matrix[ 2 ].w;
    pos = vec3( 10, 0, 5 ) * 1.25;  // TODO: Get this function working properly
    return pos;
}

/*
==========================================
main
==========================================
*/
void main() {
    vec3 normal = 2.0 * ( inNormal.xyz - vec3( 0.5 ) );
    vec3 tangent = 2.0 * ( inTangent.xyz - vec3( 0.5 ) );
    //vec3 tangent = inTangent.xyz;

    // Extract the camera position from the view matrix
    cameraPos = ExtractPos( camera.view );
    worldPos = model.model * vec4( inPosition, 1.0 );

    // Get the tangent space in world coordinates
    worldNormal = model.model * vec4( normal.xyz, 0.0 );
    worldTangent = model.model * vec4( tangent.xyz, 0.0 );

    // Project coordinate to screen
    gl_Position = camera.proj * camera.view * model.model * vec4( inPosition, 1.0 );
    fragTexCoord = inTexCoord;
}