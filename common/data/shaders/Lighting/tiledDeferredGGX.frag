#version 450

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
layout( binding = 1 ) uniform uboParms {
    buildLightTileParms_t parms;
} uniformParms;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 2 ) uniform sampler2D texGbuffer0;
layout( binding = 3 ) uniform sampler2D texGbuffer1;
layout( binding = 4 ) uniform sampler2D texGbuffer2;

/*
==========================================
Storage Buffers
==========================================
*/
const int workGroupSize = 16;

struct pointLight_t {
	vec4 mSphere;	// xyz = position, w = radius, these are in world space coordinates
	vec4 mColor;	// rgb = color, a = intensity
};
layout( std430, binding = 5 ) buffer bufferLights {
	pointLight_t pointLights[];
};

const int gMaxLightsPerTile = 125;
struct lightList_t {
	int mNumLights;
    float minDepth;
    float maxDepth;
	int mLightIds[ gMaxLightsPerTile ];
};

layout( std430, binding = 6 ) buffer bufferLightList {
	lightList_t lightLists[];
};

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec3 cameraPos;
//layout( location = 2 ) in vec4 worldPos;
//layout( location = 3 ) in vec4 worldNormal;
//layout( location = 4 ) in vec4 worldTangent;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
GGX
==========================================
*/

// Isotropic microfacet distribution function
float DistributionIsotropic( vec3 n, vec3 m, float alpha ) {
    float denom = dot( n, m ) * dot( n, m ) * ( alpha * alpha - 1.0 ) + 1.0;
    float D = alpha * alpha / ( 3.1415926 * denom * denom );
    return D;
}
// Shlick's approximation to the Fresnel equation
float FresnelShlick( vec3 N, vec3 V, float R0 ) {
    // R0 = the reflection coefficient
    // R0 = ( ( n1 - n2 ) / ( n1 + n2 ) )^2, where n1/n2 are indices of refraction
    float cosTheta = clamp( dot( N, V ), 0.0, 1.0 );
    float F = R0 + ( 1.0 - R0 ) * pow( 1.0 - cosTheta, 5.0 );
    return F;
}
float GeometricSelfShadowing( vec3 n, vec3 v, float alpha ) {
    float a2 = alpha * alpha;
    float nv = clamp( dot( n, v ), 0.0, 1.0 );
    float denom = nv + sqrt( a2 + ( 1 - a2 ) * nv * nv );
    float G = 2.0 * nv / denom;
    return G;
}
float GGX( vec3 N, vec3 V, vec3 L, float roughness, float F0 ) {
	float alpha = roughness * roughness;
	
    // Calculate half vector
	vec3 H = normalize( L + V );

    float dotNL = clamp( dot( N, L ), 0.0, 1.0 );
	
	// D - microfacet distribution function
    float D = DistributionIsotropic( N, H, alpha );
	
	// F - Shlick's Approximation for the Fresnel Reflection
//    float F = FresnelShlick( N, V, F0 );  // These are equivalent
    float F = FresnelShlick( L, H, F0 );    // These are equivalent
	
	// V - Smith approximation of the bidirectional shadowing masking
	float visibility =
            GeometricSelfShadowing( N, L, alpha ) *
            GeometricSelfShadowing( N, V, alpha );
	
	float specular = dotNL * D * F * visibility;
	return max( specular, 0.0 );
}

/*
==========================================
GammaToLinearSpace
for reference https://en.wikipedia.org/wiki/SRGB
==========================================
*
float GammaToLinearSpace( in float gamma ) {
    if ( gamma < 0.04045 ) {
        return ( gamma / 12.92 );
    }

    return pow( ( gamma + 0.055 ) / 1.055, 2.4 );
}
vec3 GammaToLinearSpace( in vec3 gamma ) {
    vec3 linear;
    linear.r = GammaToLinearSpace( gamma.r );
    linear.g = GammaToLinearSpace( gamma.g );
    linear.b = GammaToLinearSpace( gamma.b );
    return linear;
}

/*
==========================================
LinearToGammaSpace
for reference https://en.wikipedia.org/wiki/SRGB
==========================================
*
float LinearToGammaSpace( in float linear ) {
    if ( linear < 0.0031308 ) {
        return ( 12.92 * linear );
    }

    return ( pow( linear, 1.0 / 2.4 ) * 1.055 - 0.055 );
}
vec3 LinearToGammaSpace( in vec3 linear ) {
    vec3 gamma;
    gamma.r = LinearToGammaSpace( linear.r );
    gamma.g = LinearToGammaSpace( linear.g );
    gamma.b = LinearToGammaSpace( linear.b );
    return gamma;
}

/*
==========================================
main
==========================================
*/
void main() {
    ivec2 tiledCoord = ivec2( gl_FragCoord.xy ) / workGroupSize;
	int workGroupID = tiledCoord.x + tiledCoord.y * uniformParms.parms.screenWidth / workGroupSize;

    vec4 gbuffer0 = texture( texGbuffer0, fragTexCoord );
    vec4 gbuffer1 = texture( texGbuffer1, fragTexCoord );
    vec4 gbuffer2 = texture( texGbuffer2, fragTexCoord );

    if ( 0 == gbuffer2.a ) {
        discard;
    }

    vec3 worldPos = gbuffer0.xyz;
    vec3 diffuse = gbuffer1.rgb;
    vec3 normal = gbuffer2.xyz;

    float roughness = gbuffer0.a;
    float specular = gbuffer1.a;
    
    vec3 dirToCamera = normalize( cameraPos - worldPos.xyz );

    // normal needs to be transformed from texture space to world space
    //normal.x = 2.0 * ( normal.x - 0.5 );
    //normal.y = 2.0 * ( normal.y - 0.5 );
    //normal = normalize( normal );

    //vec3 worldBitangent = cross( worldNormal.xyz, worldTangent.xyz );
    //normal = normal.x * worldTangent.xyz + normal.y * worldBitangent.xyz + normal.z * worldNormal.xyz;
    //normal = -normal.x * worldTangent.xyz - normal.y * worldBitangent.xyz + normal.z * worldNormal.xyz; // I wonder if we have a bug in our tga loader

    vec4 finalColor = vec4( 0, 0, 0, 1 );

    for ( int i = 0; i < lightLists[ workGroupID ].mNumLights && i < gMaxLightsPerTile; i++ ) {
        int lightID = lightLists[ workGroupID ].mLightIds[ i ];

        vec4 lightPos = pointLights[ lightID ].mSphere;
        vec4 lightColor = pointLights[ lightID ].mColor;

        vec3 rayToLight = lightPos.xyz - worldPos.xyz;
        vec3 dirToLight = normalize( rayToLight );

        specular = GGX( normal, dirToCamera, dirToLight, roughness, specular );
        //specular.r = GGX( normal, dirToCamera, dirToLight, roughness, specular );
        //specular.g = GGX( normal, dirToCamera, dirToLight, roughness, specular );
        //specular.b = GGX( normal, dirToCamera, dirToLight, roughness, specular );

        //specular.rgb = vec3( 0 );   // Hack fix for some bad specular (not sure what's happenign, camera pos seems fine, so does normal)

        float flux = clamp( dot( normal, dirToLight ), 0.0, 1.0 );

        vec3 color;
        color.r = ( diffuse.r * flux + specular ) * lightColor.r;
        color.g = ( diffuse.g * flux + specular ) * lightColor.g;
        color.b = ( diffuse.b * flux + specular ) * lightColor.b;

        float distance = length( rayToLight );
        float intensity = clamp( 10.0f / ( distance * distance ), 0.0, 2.0 );   // TODO: switch to a gaussian
        color.rgb *= intensity;

        float t = clamp( distance / lightPos.w, 0.0, 1.0 );
        color.rgb *= mix( 1.0, 0.0, t );

        finalColor.rgb += color.rgb;
    }
    
    outColor = finalColor;

    // TODO: get rid of this and replace with SH and lightProbes
    vec3 ambient = vec3( 0.1, 0.08, 0.05 );
    outColor.rgb += ambient * diffuse.rgb;
}