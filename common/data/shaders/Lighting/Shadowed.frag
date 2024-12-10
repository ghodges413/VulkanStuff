#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform shadowParms {
    vec4 parms; // dim, time, near, far
} PushConstant;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 2 ) uniform sampler2D texGbuffer0;
layout( binding = 3 ) uniform sampler2D texGbuffer1;
layout( binding = 4 ) uniform sampler2D texGbuffer2;

layout( binding = 5 ) uniform sampler2D texShadow;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec4 fragTexCoord;
layout( location = 1 ) in mat4 shadowView;
layout( location = 5 ) in mat4 shadowProj;
layout( location = 9 ) in vec4 lightPos;
layout( location = 10 ) in vec4 cameraPos;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
DoClip
==========================================
*/
bool DoClip( vec3 xyz ) {
    if ( xyz.x < -1 || xyz.x > 1 ) {
        return true;
    }
    if ( xyz.y < -1 || xyz.y > 1 ) {
        return true;
    }
    if ( xyz.z < 0 || xyz.z > 1 ) {
        return true;
    }
    return false;
}


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

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand( in vec2 co ) {
    return fract( sin( dot( co, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

/*
==========================================
main
==========================================
*/
void main() {
    vec2 st = fragTexCoord.xy / fragTexCoord.w;
    st = st * 0.5 + 0.5;

    vec4 gbuffer0 = texture( texGbuffer0, st );
    vec4 gbuffer1 = texture( texGbuffer1, st );
    vec4 gbuffer2 = texture( texGbuffer2, st );

    if ( 0 == gbuffer2.a ) {
        discard;
    }

    vec3 worldPos = gbuffer0.xyz;
    vec3 diffuse = gbuffer1.rgb;
    vec3 normal = gbuffer2.xyz;

    float roughness = gbuffer0.a;
    float specular = gbuffer1.a;
    roughness = 0;
    specular = 1;

    vec3 dirToCamera = normalize( cameraPos.xyz - worldPos.xyz );

    vec3 dirToLight = normalize( lightPos.xyz - worldPos );
    if ( dot( normal, dirToLight ) < 0 ) {
        discard;
    }

    vec4 shadowPos = shadowProj * shadowView * vec4( worldPos, 1.0 );
    shadowPos.xyz /= shadowPos.w;
    if ( DoClip( shadowPos.xyz ) ) {
        discard;
    }
    
    vec3 shadowST = shadowPos.xyz * 0.5 + 0.5;
    float depth = texture( texShadow, shadowST.xy ).r;

    float near = 10;
    float far = 70;
    near = PushConstant.parms.z;
    far = PushConstant.parms.w;
    float delta = far - near;
    float invDelta = 1.0 / delta;
    float bias = 0.04;

    float shadowFactor = 0.0;
    if ( depth - shadowPos.z > bias * invDelta ) {
        shadowFactor = 0.2;
    }

    // Sample the shadow using offsets to smooth out jaggies
    vec2 shadowOffsets[ 4 ];
    float sizeFactor = 1.1;
    float dim = PushConstant.parms.x;
    shadowOffsets[ 0 ] = vec2( 1, 1 ) * sizeFactor / dim;
    shadowOffsets[ 1 ] = vec2(-1, 1 ) * sizeFactor / dim;
    shadowOffsets[ 2 ] = vec2(-1,-1 ) * sizeFactor / dim;
    shadowOffsets[ 3 ] = vec2( 1,-1 ) * sizeFactor / dim;

    // Use a random rotation for sampling to make the artifacting noisy (this is less offensive to the eye)
    float theta = rand( shadowST.xy ) * 2.0 * 3.1415 + PushConstant.parms.y;
    mat2 rotation;
    rotation[ 0 ][ 0 ] = cos( theta );
    rotation[ 1 ][ 0 ] = sin( theta );
    rotation[ 0 ][ 1 ] = -sin( theta );
    rotation[ 1 ][ 1 ] = cos( theta );
    
    // PCH filtering
    for ( int i = 0; i < 4; i++ ) {
        shadowOffsets[ i ] = rotation * shadowOffsets[ i ];

        float depth = texture( texShadow, shadowST.xy + shadowOffsets[ i ] ).r;
        if ( depth - shadowPos.z > bias * invDelta ) {
            shadowFactor += 0.2;
        }
    }

    specular = GGX( normal, dirToCamera, dirToLight, roughness, specular );

    // Calculate a radial falloff (this avoids harsh edegs on the sides of the light)
    float radial = length( shadowPos.xy );
    float radialFalloff = 1.0 - radial * radial * radial * radial;
    radialFalloff = clamp( radialFalloff, 0.0, 1.0 );

    // Calculate a distance falloff
    float fallOff = 1.0 - shadowPos.z;  // Add a falloff
    float dist = length( lightPos.xyz - worldPos.xyz );
    fallOff = clamp( 1.0 - ( dist / far ), 0.0, 1.0 );

    float flux = clamp( dot( normal, dirToLight ), 0.0, 1.0 ) * fallOff;
    outColor.rgb = ( diffuse * flux + specular ) * shadowFactor * radialFalloff;
    outColor.a = 1.0;
}