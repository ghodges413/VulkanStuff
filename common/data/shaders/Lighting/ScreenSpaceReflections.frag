#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( binding = 0 ) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 1 ) uniform sampler2D texGbuffer0;
layout( binding = 2 ) uniform sampler2D texGbuffer1;
layout( binding = 3 ) uniform sampler2D texGbuffer2;

layout( binding = 4 ) uniform sampler2D texColorBuffer;
layout( binding = 5 ) uniform samplerCube texLightProbe;

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

    float roughness = gbuffer0.a;
    float metalness = gbuffer1.a;

    vec3 cameraPos = ExtractPos( camera.view );
    vec3 rayView = normalize( worldPos - cameraPos );
    vec3 rayReflected = reflect( rayView, normal );

    vec3 dirToLight = rayReflected;
    vec3 dirToCamera = -rayView;

    float specular = GGX( normal, dirToCamera, dirToLight, roughness, metalness );
    specular = clamp( specular, 0.0, 1.0 );

#if 0   // Enable for debug rendering (forces the floor to be shiny)
    if ( normal.z < 0.95 ) {
        discard;
    }
    normal = vec3( 0, 0, 1 );
    roughness = 0;
    specular = 1;
#endif
    if ( 0 == specular ) {
        discard;
    }

    // If the reflected ray is headed away from the scene, and towards the camera, then skip the trace
    bool skipTrace = false;
    if ( dot( rayReflected, rayView ) < 0 ) {
        skipTrace = true;
    }

    vec3 color = vec3( 0 );
    float blend = 0.0;

    // Raymarch the reflected ray until it hits something or runs out of max steps
    float stepSize = 1.0;
    float threshold = 0.1;
    const int maxIters = 30;
    vec3 samplePos = worldPos;
    for ( int i = 0; i < maxIters && !skipTrace; i++ ) {
        vec3 prevPos = samplePos;
        samplePos += rayReflected * stepSize;

        // Project this into screen space and sample
        vec4 coord = camera.proj * camera.view * vec4( samplePos.xyz, 1.0 );
        coord.xy /= coord.w;

        // If the trace goes offscreen, then stop tracing
        if ( DoClip( coord.xy ) ) {
            break;
        }

        vec2 st = coord.xy * 0.5 + 0.5;
        vec4 worldPos2 = texture( texGbuffer0, st );
        
        float distWorld = length( worldPos2.xyz - cameraPos );
        float distSample = length( samplePos - cameraPos );
        float deltaDist = distSample - distWorld;

        // If the trace has hit something, but it's penetrated too deep
        // then rewind and make the step size smaller
        if ( deltaDist > 0 && deltaDist > threshold ) {//&& i < maxIters - 2 ) {
            samplePos = prevPos + rayReflected * threshold * 0.1;
            stepSize *= 0.5;
            continue;
        }

        // If the trace has hit something then sample the color buffer and stop tracing
        if ( distSample >= distWorld || 0 == gbuffer2.a ) {
            color = texture( texColorBuffer, st ).rgb;

            float r2 = coord.x * coord.x + coord.y * coord.y;
            r2 = r2 * r2;
            blend = clamp( 1.0 - r2 * r2, 0.0, 1.0 );

            break;
        }
    }

    outColor.rgb = color * blend * specular;
    roughness *= roughness;
    roughness *= roughness;
    outColor += ( 1.0 - blend ) * textureLod( texLightProbe, rayReflected, roughness * 8.0 ) * specular;
    outColor.a = 1.0;
}