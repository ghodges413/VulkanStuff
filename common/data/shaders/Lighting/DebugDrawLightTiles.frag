// This lines tells OpenGL which version of GLSL we're using
#version 430

/*
==========================================
Uniforms
==========================================
*/

struct buildLightTileParms_t {
	mat4 matView;
	mat4 matProjInv;

	int screenWidth;
	int screenHeight;
	int maxLights;
	int pad0;
};
layout( binding = 0 ) uniform uboParms {
    buildLightTileParms_t parms;
} uniformParms;

/*
==========================================
Storage Buffers
==========================================
*/

const int gMaxLightsPerTile = 127;
struct lightList_t {
	int mNumLights;
	int mLightIds[ gMaxLightsPerTile ];
};

layout( std430, binding = 1 ) buffer bufferLightList {
	lightList_t lightLists[];
};

/*
==========================================
Output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
Constants
==========================================
*/

const int workGroupSize = 16;

/*
==========================
main
==========================
*/
void main() {
	ivec2 tiledCoord = ivec2( gl_FragCoord.xy ) / workGroupSize;
	int workGroupID = tiledCoord.x + tiledCoord.y * uniformParms.parms.screenWidth / workGroupSize;
	
	outColor.rgb = vec3( 0.0 );
	outColor.a = 1.0;
	
	int numLights = lightLists[ workGroupID ].mNumLights;
	outColor.r = float( numLights ) / float( gMaxLightsPerTile );
	outColor.r *= 2.0;
}
