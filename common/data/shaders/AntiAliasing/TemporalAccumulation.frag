#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform pushDims {
    vec4 invDims;
} PushConstant;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 0 ) uniform sampler2D texFrame;
layout( binding = 1 ) uniform sampler2D texHistory;
layout( binding = 2 ) uniform sampler2D texVelocity;
//layout( binding = 3 ) uniform sampler2D texHistoryDepth;
//layout( binding = 4 ) uniform sampler2D texDepth;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;

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
    vec2 st = fragTexCoord;
    vec4 velocity = texture( texVelocity, st ) + vec4( st.x, st.y, 0, 0 );
    vec4 colorFrame = texture( texFrame, st );

    float invX = PushConstant.invDims.x;
    float invY = PushConstant.invDims.y;

    // This heuristic is to clamp the history color to the neighborhood of the current frame's pixels
    vec4 colorNeighbors[ 8 ];
    colorNeighbors[ 0 ] = texture( texFrame, st + vec2( invX, 0 ) );
    colorNeighbors[ 1 ] = texture( texFrame, st + vec2( -invX, 0 ) );
    colorNeighbors[ 2 ] = texture( texFrame, st + vec2( 0, invY ) );
    colorNeighbors[ 3 ] = texture( texFrame, st + vec2( 0, -invY ) );

    colorNeighbors[ 4 ] = texture( texFrame, st + vec2( invX, invY ) );
    colorNeighbors[ 5 ] = texture( texFrame, st + vec2( -invX, invY ) );
    colorNeighbors[ 6 ] = texture( texFrame, st + vec2( -invX, -invY ) );
    colorNeighbors[ 7 ] = texture( texFrame, st + vec2( invX, -invY ) );

    vec4 colorMin0 = min( colorNeighbors[ 0 ], min( colorNeighbors[ 1 ], min( colorNeighbors[ 2 ], colorNeighbors[ 3 ] ) ) );
    vec4 colorMax0 = max( colorNeighbors[ 0 ], max( colorNeighbors[ 1 ], max( colorNeighbors[ 2 ], colorNeighbors[ 3 ] ) ) );

    vec4 colorMin1 = min( colorNeighbors[ 4 ], min( colorNeighbors[ 5 ], min( colorNeighbors[ 6 ], colorNeighbors[ 7 ] ) ) );
    vec4 colorMax1 = max( colorNeighbors[ 4 ], max( colorNeighbors[ 5 ], max( colorNeighbors[ 6 ], colorNeighbors[ 7 ] ) ) );

    vec4 colorMin = min( colorMin0, colorMin1 );
    vec4 colorMax = max( colorMax0, colorMax1 );

    vec2 stHistory = velocity.xy;
    vec4 colorHistory = texture( texHistory, stHistory );

    colorHistory = clamp( colorHistory, colorMin, colorMax );
    outColor = mix( colorFrame, colorHistory, 0.9 );

    // If the texel was outside the viewport, then the history is invalid
    if ( stHistory.x < 0 || stHistory.x > 1 || stHistory.y < 0 || stHistory.y > 1 ) {
        outColor = colorFrame;
    }

    // The below heuristic works by invalidating history pixels that were
    // occluded.  While this does work to prevent ghosting, simply clamping
    // the history pixel to the neighborhood of the current frame's pixel
    // works better in practice.
#if 0
    float depth = texture( texDepth, st ).r;
    float depthHistory = texture( texHistoryDepth, stHistory ).r;

    // If this texel was occluded in the last frame, then the history is invalid
    if ( velocity.z - depthHistory > 0.0001 ) {
        outColor = colorFrame;
    }

    // Don't perform TAA on the skybox
    if ( 1.0 == depthHistory ) {
        outColor = colorFrame;
    }
    if ( 1.0 == depth ) {
        outColor = colorFrame;
    }
#endif
}