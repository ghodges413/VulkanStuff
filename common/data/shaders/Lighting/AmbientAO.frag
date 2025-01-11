#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform bounds {
    vec4 mins;
    vec4 maxs;

    float aoeRadius;
    int maxSamples;
    float time;
    float pad;
} PushConstant;

layout( binding = 0 ) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 1 ) uniform sampler3D texAmbientRed;
layout( binding = 2 ) uniform sampler3D texAmbientGreen;
layout( binding = 3 ) uniform sampler3D texAmbientBlue;

layout( binding = 4 ) uniform sampler2D texGbuffer0;
layout( binding = 5 ) uniform sampler2D texGbuffer1;
layout( binding = 6 ) uniform sampler2D texGbuffer2;

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
====================================================================================

Calculate Ambient Occlusion

====================================================================================
*/

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand( in vec2 co ) {
    return fract( sin( dot( co, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

/*
==========================================
DoClip
==========================================
*/
bool DoClip( vec2 xy ) {
    if ( xy.x < -1 || xy.x > 1 ) {
        return true;
    }
    if ( xy.y < -1 || xy.y > 1 ) {
        return true;
    }
    return false;
}

/*
==========================================
OrthoBasis
==========================================
*/
void OrthoBasis( in vec3 norm, out vec3 tang, out vec3 bitang ) {
    tang = vec3( 1, 0, 0 );

    float tmp = dot( tang, norm );
    if ( tmp * tmp > 0.8 ) {
        tang = vec3( 0, 0, 1 );
    }

    bitang = normalize( cross( norm, tang ) );
    tang = normalize( cross( bitang, norm ) );
}

#define MAX_VECTORS 17
#define DD 0.03
#define DD2 0.05
vec3 testVectors[ MAX_VECTORS ] = {
    vec3( 0, 0, DD2 ),
    vec3( DD, DD, DD ),    
    vec3(-DD, DD, DD ),
    vec3(-DD,-DD, DD ),
    vec3( DD,-DD, DD ),
    vec3( DD, DD2, DD ),
    vec3(-DD, DD2, DD ),
    vec3(-DD,-DD2, DD ),
    vec3( DD,-DD2, DD ),
    vec3( DD2, DD, DD ),
    vec3(-DD2, DD, DD ),
    vec3(-DD2,-DD, DD ),
    vec3( DD2,-DD, DD ),
    vec3( DD, 0, DD2 ),
    vec3(-DD, 0, DD2 ),
    vec3( 0, DD, DD2 ),
    vec3( 0,-DD, DD2 ),
};

/*
==========================================
CalculateOcclusion
==========================================
*/
float CalculateOcclusion() {
    vec4 gbuffer0 = texture( texGbuffer0, fragTexCoord );
    vec4 gbuffer1 = texture( texGbuffer1, fragTexCoord );
    vec4 gbuffer2 = texture( texGbuffer2, fragTexCoord );

    if ( 0 == gbuffer2.a ) {
        discard;
    }

    vec3 worldPos = gbuffer0.xyz;
    vec3 diffuse = gbuffer1.rgb;
    vec3 normal = gbuffer2.xyz;

    vec3 tang;
    vec3 bitang;
    OrthoBasis( normal, tang, bitang );

    float occlusionFactor = 0;
    float aoeRadius = PushConstant.aoeRadius;

    // Use a random rotation for sampling to make the artifacting noisy (this is less offensive to the eye)
    float theta = rand( fragTexCoord ) * 2.0 * 3.1415 + PushConstant.time;
    mat2 rotation;
    rotation[ 0 ][ 0 ] = cos( theta );
    rotation[ 1 ][ 0 ] = sin( theta );
    rotation[ 0 ][ 1 ] = -sin( theta );
    rotation[ 1 ][ 1 ] = cos( theta );

    // Sample the area around the fragment for occlusion
    float numSamples = 0;
    int maxSamples = MAX_VECTORS;
    if ( PushConstant.maxSamples < MAX_VECTORS ) {
        maxSamples = PushConstant.maxSamples;
    }

    for ( int i = 0; i < maxSamples; i++ ) {
        vec3 testVector = testVectors[ i ];
        testVector.xy = rotation * testVector.xy;
        
        vec3 sampleVector = tang * testVector.x + bitang * testVector.y + normal * testVector.z;
        vec3 samplePos = worldPos + sampleVector;

        // Project this into screen space and sample
        vec4 coord = camera.proj * camera.view * vec4( samplePos.xyz, 1.0 );
        coord.xy /= coord.w;

        // If the trace goes offscreen, then stop tracing
        if ( DoClip( coord.xy ) ) {
            continue;
        }

        vec2 st = coord.xy * 0.5 + 0.5;
        vec4 worldPos2 = texture( texGbuffer0, st );

        // If the sampled point is outside the area of effect
        vec3 ray = worldPos2.xyz - worldPos;
        float dist = length( ray );
        if ( dist > aoeRadius ) {
            continue;
        }

        // Calculate the occlusion based on distance
        float occluder = ( aoeRadius - dist ) / aoeRadius;
        occluder = clamp( occluder, 0.0, 1.0 );

        // Now we need to determine if the occluder is behind or in front... anything behind is not an occluder
        // If the occluder is behind, then it should not contribute to occlusion
        {
            ray = normalize( ray );
            float d = dot( ray, normal );   // d < 0 is behind, d > 0 is in front
            d = clamp( d, 0.0, 1.0 );
            occluder *= d;
        } 

        // If the normal points in the same direction as the occluder's normal,
        // then don't occlude (this prevents adjacent things from occluding and fixes a really ugly sampling bug)
        {
            vec4 normal2 = texture( texGbuffer2, st );
            float d = dot( normal, normal2.xyz );
            d = 1.0 - clamp( d * d * d * d, 0.0, 1.0 );
            occluder *= d;
        }

        occlusionFactor += occluder;
        numSamples += 1.0;
    }
/*
    if ( 0 == numSamples ) {
        outColor = vec4( 1 );
    } else {
        occlusionFactor /= numSamples;
        outColor.rgb = vec3( 1.0 - occlusionFactor );
        outColor.a = 1.0;
    }
*/
    return clamp( 1.0 - occlusionFactor, 0.0, 1.0 );
}

/*
====================================================================================

Calculate Ambient

====================================================================================
*/

/*
==========================================
ColorFromSH
==========================================
*/
vec3 ColorFromSH( vec3 dir, in vec3 ylm00, in vec3 ylm11, in vec3 ylm10, in vec3 ylm1n1 ) {
    // These ylm's were calculated from my linked program on my github
    //vec3 ylm00 = vec3( 0.285016, 0.285016, 0.209534 );
    //vec3 ylm11 = vec3( 0.194225, -0.001365, 0.000291 );
    //vec3 ylm10 = vec3( -0.001073, -0.001073, -0.105487 );
    //vec3 ylm1n1 = vec3( -0.003512, -0.196372, 0.193152 );
    //vec3 ylm22 = vec3( 0 );
    //vec3 ylm21 = vec3( 0 );
    //vec3 ylm20 = vec3( 0 );
    //vec3 ylm2n1 = vec3( 0 );
    //vec3 ylm2n2 = vec3( 0 );

    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;

    vec3 color = ylm00 * c4
        + ( ylm11 * dir.x + ylm1n1 * dir.y + ylm10 * dir.z ) * 2.0 * c2;
        //- ylm20 * c5
        //+ ylm20 * c3 * dir.z * dir.z
        //+ ylm22 * c1 * ( dir.x * dir.x - dir.y * dir.y )
        //+ ( ylm2n2 * dir.x * dir.y + ylm21 * dir.x * dir.z + ylm2n1 * dir.y * dir.z ) * 2.0 * c1;
    return color;
}

/*
==========================================
main
==========================================
*/
void main() {
    vec4 gbuffer0 = texture( texGbuffer0, fragTexCoord );
    vec4 gbuffer1 = texture( texGbuffer1, fragTexCoord );
    vec4 gbuffer2 = texture( texGbuffer2, fragTexCoord );

    if ( 0 == gbuffer2.a ) {
        discard;
    }

    vec3 worldPos = gbuffer0.xyz;
    vec3 diffuse = gbuffer1.rgb;
    vec3 normal = gbuffer2.xyz;

    // These mins/maxs are the bounds of the 3d texture holding the nodes
    vec3 mins = PushConstant.mins.xyz;
    vec3 maxs = PushConstant.maxs.xyz;

    // Convert the world space position to uvw coords of the 3d textures
    vec3 dims = maxs - mins;
    vec3 samplePos = worldPos - mins;
    vec3 uvw = samplePos / dims;

    // Sample textures
    vec4 red = texture( texAmbientRed, uvw );
    vec4 green = texture( texAmbientGreen, uvw );
    vec4 blue = texture( texAmbientBlue, uvw );

    // Convert to spherical harmonic components
    vec3 ylm00 = vec3( red.x, green.x, blue.x );
    vec3 ylm11 = vec3( red.y, green.y, blue.y );
    vec3 ylm10 = vec3( red.z, green.z, blue.z );
    vec3 ylm1n1 = vec3( red.w, green.w, blue.w );

    vec3 ambientColor = ColorFromSH( normal, ylm00, ylm11, ylm10, ylm1n1 );
    float occlusion = CalculateOcclusion();

    outColor.rgb = ambientColor * diffuse * occlusion;
    outColor.a = 1.0;
}