#version 450

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;
//layout( location = 0 ) out float fragmentdepth;

/*
==========================================
main
==========================================
*/
void main() {
    outColor = vec4( 0 );
    gl_FragDepth = gl_FragCoord.z;
    //vec4 finalColor = vec4( 1, 0, 0, 1 );
    //outColor = finalColor;
//	outColor = vec4( gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0 );
//	fragmentdepth = gl_FragCoord.z;
}