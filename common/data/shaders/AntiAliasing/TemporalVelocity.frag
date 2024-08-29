#version 450

/*
==========================================
Uniforms
==========================================
*/

/*
==========================================
Samplers
==========================================
*/

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec4 fragTexCoord;
layout( location = 1 ) in vec4 fragTexCoordOld;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
main
==========================================
*/
void main() {
    vec4 coord = fragTexCoord / fragTexCoord.w;
    coord.xy = ( coord.xy + 1.0 ) * 0.5;

    vec4 coordOld = fragTexCoordOld / fragTexCoordOld.w;
    coordOld.xy = ( coordOld.xy + 1.0 ) * 0.5;

    vec2 delta = coordOld.xy - coord.xy;
    outColor.x = delta.x;
    outColor.y = delta.y;
    outColor.z = coordOld.z;    // Store the old fragment's depth, to know if it was occluded in the previous frame
    outColor.w = fragTexCoordOld.w;
}