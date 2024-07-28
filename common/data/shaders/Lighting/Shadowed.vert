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

layout( binding = 1 ) uniform uboShadowCamera {
    mat4 view;
    mat4 proj;
} shadowCamera;

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
layout( location = 1 ) out mat4 shadowView;
layout( location = 5 ) out mat4 shadowProj;
layout( location = 9 ) out vec4 shadowPos;
layout( location = 10 ) out vec4 cameraPos;

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
    mat4 invProj = inverse( shadowCamera.proj );
    mat4 invView = inverse( shadowCamera.view );

    // Transform the vertex position from NDC space to view space
    vec4 vert = invProj * vec4( inPosition, 1.0 );
    vert.xyz /= vert.w;
    vert.w = 1.0;

    // Transform the vertex position from view space to world space
    vert = invView * vert;

    // Project the model vert into screen space now
    vec4 vert2 = camera.proj * camera.view * vec4( vert.xyz, 1.0 );

    // Project coordinate to screen
    gl_Position = vert2;
    fragTexCoord = vert2;

    shadowView = shadowCamera.view;
    shadowProj = shadowCamera.proj;

    shadowPos = vec4( ExtractPos( shadowCamera.view ), 1.0 );
    cameraPos = vec4( ExtractPos( camera.view ), 1.0 );
}