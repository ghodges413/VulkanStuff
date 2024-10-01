#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform pushColor {
    vec4 color;
    vec4 time;
} PushConstant;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec4 worldPos;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
pcg3d
// https://www.shadertoy.com/view/XlGcRh
// http://www.jcgt.org/published/0009/03/02/
==========================================
*/
uvec3 pcg3d( uvec3 v ) {
    v = v * 1664525u + 1013904223u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    v ^= v >> 16u;

    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    return v;
}

/*
==========================================
GetRandom
==========================================
*/
float GetRandom() {
    float time = PushConstant.time.x;
    vec3 pos = abs( worldPos.xyz ) * 100000.0;

    uint pc = pcg3d( uvec3( pos + time ) ).x;
    float val = float( pc ) / float( 0xffffffffu );
    return val;
}

/*
==========================================
main
==========================================
*/
void main() {
    vec4 color = PushConstant.color;

    float random = GetRandom();
    if ( random < color.a ) {
        discard;
    }

    outColor = color;
}