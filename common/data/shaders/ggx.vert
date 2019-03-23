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

    /*
    vec4 viewRow0;
    vec4 viewRow1;
    vec4 viewRow2;
    vec4 viewRow3;

    vec4 projRow0;
    vec4 projRow1;
    vec4 projRow2;
    vec4 projRow3;
    */
} camera;
layout( binding = 1 ) uniform uboModel {
    mat4 model;
    /*
    vec4 modelRow0;
    vec4 modelRow1;
    vec4 modelRow2;
    vec4 modelRow3;
    */
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

/*
==========================================
TransformPoint
==========================================
*/
vec4 TransformPoint( in vec4 row0, in vec4 row1, in vec4 row2, in vec4 row3, in vec4 point ) {
    vec4 pt;
    pt.x = dot( row0, point );
    pt.y = dot( row1, point );
    pt.z = dot( row2, point );
    pt.w = dot( row3, point );
    return pt;
}

/*
==========================================
main
==========================================
*/
void main() {
    //vec3 tangent = 2.0 * inTangent.xyz - vec3( 1, 1, 1 );
    vec3 tangent = inTangent.xyz;

    // Extract the camera position from the view matrix
    cameraPos = camera.view[ 3 ].xyz;
    worldPos = model.model * vec4( inPosition, 1.0 );

    // Get the tangent space in world coordinates
    worldNormal = model.model * vec4( inNormal.xyz, 0.0 );
    worldTangent = model.model * vec4( tangent.xyz, 0.0 );

    // Project coordinate to screen
    gl_Position = camera.proj * camera.view * model.model * vec4( inPosition, 1.0 );
//    gl_Position = vec4( inPosition, 1.0 ) * model.model * camera.view * camera.proj;
    fragTexCoord = inTexCoord;
}