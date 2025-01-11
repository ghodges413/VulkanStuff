#version 450

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor0;
layout( location = 1 ) out vec4 outColor1;
layout( location = 2 ) out vec4 outColor2;

/*
==========================================
main
==========================================
*/
void main() {
    outColor0 = vec4( 0 );
    outColor1 = vec4( 0 );
    outColor2 = vec4( 0 );
    gl_FragDepth = gl_FragCoord.z;
}