#version 450

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec4 modelPos;
layout( location = 1 ) in vec4 timeTime;
layout( location = 2 ) in vec3 camPos;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;



#define ABSORPTION_COEFFICIENT 0.5


#define UNIFORM_FOG_DENSITY 1

#define BETA vec3( 0.7, 0.7, 0.9 )
#define PI 3.14159265359


/*
====================================================================================

Noise Functions

====================================================================================
*/

/*
==========================================
hash1
Taken from Inigo Quilez's Rainforest ShaderToy:
https://www.shadertoy.com/view/4ttSWf
==========================================
*/
float hash1( float n ) {
    return fract( n * 17.0 * fract( n * 0.3183099 ) );
}

/*
==========================================
noise
Taken from Inigo Quilez's Rainforest ShaderToy:
https://www.shadertoy.com/view/4ttSWf
==========================================
*/
float noise( in vec3 x ) {
    vec3 p = floor( x );
    vec3 w = fract( x );
    
    vec3 u = w * w * w * ( w * ( w * 6.0 - 15.0 ) + 10.0 );
    
    float n = p.x + 317.0 * p.y + 157.0 * p.z;
    
    float a = hash1( n + 0.0 );
    float b = hash1( n + 1.0 );
    float c = hash1( n + 317.0 );
    float d = hash1( n + 318.0 );
    float e = hash1( n + 157.0 );
	float f = hash1( n + 158.0 );
    float g = hash1( n + 474.0 );
    float h = hash1( n + 475.0 );

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0 + 2.0 * ( k0 + k1 * u.x + k2 * u.y + k3 * u.z + k4 * u.x * u.y + k5 * u.y * u.z + k6 * u.z * u.x + k7 * u.x * u.y * u.z );
}

const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );

/*
==========================================
fbm_4
Taken from Inigo Quilez's Rainforest ShaderToy:
https://www.shadertoy.com/view/4ttSWf
==========================================
*/
float fbm_4( in vec3 x ) {
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;

    int iFrame = 0;
    for ( int i = min( 0, iFrame ); i < 4; i++ ) {
        float n = noise( x );
        a += b * n;
        b *= s;
        x = f * m3 * x;
    }

	return a;
}


/*
====================================================================================

Signed Distance Functions

https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

====================================================================================
*/

float sdCeiling( vec3 p ) {
	return -p.z;
}
float sdFloor( vec3 p ) {
	return p.z;
}
float sdSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5 * ( d2 - d1 ) / k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k * h * ( 1.0 - h ); 
}

float sdSphere( vec3 p, float s ) {
    return length( p ) - s;
}
float sdSphere( vec3 p, vec3 origin, float s ) {
    p = p - origin;
    return sdSphere( p, s );
}

float sdSphereInverted( vec3 p, vec3 origin, float s ) {
    return -sdSphere( p, origin, s );
}

#define SPHERE_RADIUS 150.0f
/*
==========================================
QueryVolumetricDistanceField
==========================================
*/
float QueryVolumetricDistanceField( in vec3 pos ) {
    float time = timeTime.x;

    float sdfValue = sdSphereInverted( pos, vec3( 0 ), SPHERE_RADIUS );
//    float sdfFloor = sdFloor( pos + vec3( 0, 0, 10 ) );
//    float sdfCeiling = sdCeiling( pos + vec3( 0, 0, -100 ) );
//    sdfValue = sdSmoothUnion( sdfValue, sdfFloor, 3.0 );
//    sdfValue = sdSmoothUnion( sdfValue, sdfCeiling, 3.0 );

    //vec3 fbmCoord = ( pos + 2.0 * vec3( 0.0, 0.0, -time ) ) / 1.5f;
    //vec3 fbmCoord = ( pos + 2.0 * vec3( time, 0.0, -time ) ) / 1.5f;
    vec3 fbmCoord = ( pos + 2.0 * vec3( 0.0, 0.0, -time ) ) * 0.5;// / 1.5f;
    float sdfNoise = 7.0 * fbm_4( fbmCoord / 3.2 );

    sdfValue += sdfNoise;

    return sdfValue;
}

/*
==========================================
GetFogDensity
==========================================
*/
float GetFogDensity( vec3 position, float sdfDistance ) {
    const float maxSDFMultiplier = 1.0;
    float sdfMultiplier = 0.0;

    // Calculate the sdf mulitplier if we're inside the signed distance object
    if ( sdfDistance < 0.0 ) {
        sdfMultiplier = min( abs( sdfDistance ), maxSDFMultiplier );
    }
 
#if UNIFORM_FOG_DENSITY
    return sdfMultiplier;
#else
   return sdfMultiplier * abs( fbm_4( position / 6.0 ) + 0.5 );
#endif
}

/*
==========================================
BeerLambert
==========================================
*/
float BeerLambert( float absorption, float dist ) {
    return exp( -absorption * dist );
}

/*
==========================================
Luminance
==========================================
*/
float Luminance( vec3 color ) {
    return ( color.r * 0.3 ) + ( color.g * 0.59 ) + ( color.b * 0.11 );
}

/*
==========================================
IsColorInsignificant
==========================================
*/
bool IsColorInsignificant( vec3 color ) {
    const float minValue = 0.009;
    return Luminance( color ) < minValue;
}

/*
==========================================
BlackBodySpectrum
==========================================
*/
vec3 BlackBodySpectrum( float t ) {
    const float temperature = 2200.0;
    t *= temperature;

    float u = ( 0.860117757 + 1.54118254e-4 * t + 1.28641212e-7 * t * t ) / ( 1.0 + 8.42420235e-4 * t + 7.08145163e-7 * t * t );
    float v = ( 0.317398726 + 4.22806245e-5 * t + 4.20481691e-8 * t * t ) / ( 1.0 - 2.89741816e-5 * t + 1.61456053e-7 * t * t );

    float x = 3.0 * u / ( 2.0 * u - 8.0 * v + 4.0 );
    float y = 2.0 * v / ( 2.0 * u - 8.0 * v + 4.0 );
    float z = 1.0 - x - y;

    float Y = 1.0;
    float X = Y / y * x;
    float Z = Y / y * z;

    mat3 XYZtoRGB = mat3(
        3.2404542, -1.5371385, -0.4985314,
        -0.9692660,  1.8760108,  0.0415560,
        0.0556434, -0.2040259,  1.0572252
    );

    return max( vec3( 0.0 ), ( vec3( X, Y, Z ) * XYZtoRGB ) * pow( t * 0.0004, 4.0 ) );
}

/*
 ===============================
 IntersectRaySphere
 always t1 <= t2
 if t1 < 0 && t2 > 0 ray is inside
 if t1 < 0 && t2 < 0 sphere is behind ray origin
 recover the 3D position with p = rayStart + t * rayDir
 ===============================
 */
bool IntersectRaySphere( in vec3 rayStart, in vec3 rayDir, in vec4 sphere, out float t1, out float t2 ) {
    // Note:    If we force the rayDir to be normalized,
    //          then we can get an optimization where
    //          a = 1, b = m.
    //          Which would decrease the number of operations
    vec3 m = sphere.xyz - rayStart;
    float a   = dot( rayDir, rayDir );
    float b   = dot( m, rayDir );
    float c   = dot( m, m ) - sphere.a * sphere.a;
    
    float delta = b * b - a * c;
    float invA = 1.0f / a;
    
    if ( delta < 0 ) {
        // no real solutions exist
        return false;
    }
    
    float deltaRoot = sqrt( delta );
    t1 = invA * ( b - deltaRoot );
    t2 = invA * ( b + deltaRoot );

    return true;
}

/*
==========================================
SampleCloudSDF
Low resolution cloud geometry
==========================================
*/
float SampleCloudSDF2( vec3 p ) {
    float sdf = QueryVolumetricDistanceField( p );
    return sdf;
}

/*
==========================================
SampleCloudNoise
High resolution noise/detail
==========================================
*/
float SampleCloudNoise( float sdfSample, vec3 samplePos ) {
    /*
    float cloud = clamp( -sdfSample / 20.0, 0.0, 1.0 );
    if ( cloud > 0.0 ) {
        vec3 velocity = vec3( 1, 2, 3 );
        vec3 offset = velocity * time;
        vec3 worley = SampleWorleyNoise( samplePos + offset, 0.4 ) * 0.8;

        cloud = Remap( cloud, worley.x, 1.0, 0.0, 1.0 );
        cloud = clamp( cloud, 0.0, 1.0 );
    }
    return cloud * 0.5;
    */
    //float GetFogDensity( vec3 position, float sdfDistance )
    return GetFogDensity( samplePos, sdfSample );
}


/*
==========================================
PathDensity
Accumulates the cloud density along the path
==========================================
*/
float PathDensity( vec3 rayPos, vec3 rayDir ) {
    int maxSteps = 12;
    float maxDistance = 50.0;
    float ds = maxDistance / maxSteps;
    float density = 0.0;
    float s = 0.0;

    // Accumulate the cloud density in the direction of the ray
    for ( int i = 0; i < maxSteps; i++ ) {
        vec3 samplePos = rayPos + rayDir * s;

        float sdfSample = SampleCloudSDF2( samplePos );
        float densitySample = SampleCloudNoise( sdfSample, samplePos );

        density += densitySample * ds;
        s += ds;
    }

    return density;
}


float HenyeyGreenstein( float g, float costh ) {
    return ( 1.0 / ( 4.0 * PI ) ) * ( ( 1.0 - g * g ) / pow( 1.0 + g * g - 2.0 * g * costh, 1.5 ) );
}

float IsotropicPhaseFunction( float g, float costh ) {
    return 1.0 / ( 4.0 * PI );
}

float BeerPowder( float coeff, float depth ) {
    float powderSugarEffect = 1.0f - exp( -depth * 2.0f );
    float beersLaw = exp( -depth );
    float lightEnergy = 2.0f * beersLaw * powderSugarEffect;
    return lightEnergy;
}

float HenyeyGreenstein2( float g, float costh ) {
    return mix( HenyeyGreenstein( -g, costh ), HenyeyGreenstein( g, costh ), 0.7 );
}

float PhaseFunction( float g, float costh ) {
#if 1
    return IsotropicPhaseFunction( g, costh );
#else
    return HenyeyGreenstein2( g, costh );
#endif
}


/*
==========================================
MultipleOctaveScattering
// Adapted from: https://twitter.com/FewesW/status/1364629939568451587/photo/1
// Original Paper: https://magnuswrenninge.com/wp-content/uploads/2010/03/Wrenninge-OzTheGreatAndVolumetric.pdf
==========================================
*/
vec3 MultipleOctaveScattering( float density, float cosTheta ) {
    float attenuation = 0.2;
    float contribution = 0.2;
    float phaseAttenuation = 0.5;

    float a = 1.0;
    float b = 1.0;
    float c = 1.0;
    float g = 0.85;
    const float scatteringOctaves = 4.0;

    vec3 luminance = vec3( 0.0 );

    for ( float i = 0.0; i < scatteringOctaves; i++ ) {
        float phaseFunction = PhaseFunction( 0.3 * c, cosTheta );
        vec3 beers = exp( -density * BETA * a );

        luminance += b * phaseFunction * beers;

        a *= attenuation;
        b *= contribution;
        c *= ( 1.0 - phaseAttenuation );
    }
    return luminance;
}

/*
==========================================
LightEnergy
Calculates the amount of sun light entering the sample position in the cloud
==========================================
*/
vec3 LightEnergy( vec3 samplePos, float cosTheta, vec3 sunDir ) {
    float density = PathDensity( samplePos, sunDir );
    //lightVisibility *= BeerLambert( ABSORPTION_COEFFICIENT * density, marchSize );
    vec3 beersLaw = MultipleOctaveScattering( density, cosTheta );
    vec3 powderSugarEffect = 1.0 - exp( -density * 2.0 * BETA );
    vec3 lightEnergy = 2.0f * beersLaw;// * powderSugarEffect;

    //float depth = abs( signedDistance - emissiveDepth );
    float depth = abs( length( samplePos ) - SPHERE_RADIUS - 10.0f );
    float temperature = depth * 0.15;
    vec3 emissiveColor = BlackBodySpectrum( temperature );

    lightEnergy *= emissiveColor;

    return lightEnergy;
}

/*
==========================================
RayMarch
ray marches through the cloud scape
==========================================
*/
vec3 RayMarch( vec3 rayPos, vec3 rayDir ) {
    vec4 sphere = vec4( 0, 0, 0, SPHERE_RADIUS - 10.0f );

    float t0;
    float t1;
    if ( !IntersectRaySphere( rayPos, rayDir, sphere, t0, t1 ) ) {
        discard;
    }
    float tStart = t1; // This assumes that the camera is inside the sphere

    sphere.a = SPHERE_RADIUS + 10.0f;
    if ( !IntersectRaySphere( rayPos, rayDir, sphere, t0, t1 ) ) {
        discard;
    }
    float tEnd = t1; // This assumes that the camera is inside the sphere

    vec3 sunLightColour = vec3( 1.0 );
    vec3 sunLight = sunLightColour * 50.0;
    vec3 ambient = sunLightColour * 0.1;

    //float cosTheta = dot( rayDir, sunDir );

    const int maxSamples = 32;//256;
    float minStep = ( t1 - t0 ) / maxSamples;
    minStep = max( 1.0, minStep );
    minStep = 1.0;

    //
    //  Ray march through the cloudscape
    //
    float t = tStart;
    vec3 transmittance = vec3( 1 );
    vec3 scattering = vec3( 0 );
    while ( t < tEnd ) {
        vec3 samplePos = rayPos + rayDir * t;
        float sdfSample = SampleCloudSDF2( samplePos );
        float dt = ( sdfSample > minStep ) ? sdfSample : minStep;
        t += dt;

        // Not in a cloud, skip expensive sampling
        if ( sdfSample >= 0.0 ) {
            continue;
        }

        // Sample the high detail cloud density from the worley noise
        float density = SampleCloudNoise( sdfSample, samplePos );
        if ( density <= 0.01 ) {
            // There's no cloud here, it's just empty sky.  Therefore there's no scattering event.
            continue;
        }

        vec3 dirToLight = normalize( samplePos );   // Make sure the direction to the light is the shortest distance to the outer sphere
        float cosTheta = dot( rayDir, dirToLight );

        // Calculate the light entering this sample position
        vec3 energy = LightEnergy( samplePos, cosTheta, dirToLight );
        vec3 luminance = ambient + sunLight * energy;

        // Calcualte the light scattered into the camera from this position
        vec3 transmission = exp( -density * BETA * dt );
        vec3 scatter = luminance * ( 1.0 - transmission );

        // Accumulate light
        scattering += transmittance * scatter;
        transmittance *= transmission;

        // Early out if the transmittance is opaque
        if ( IsColorInsignificant( transmittance ) ) {
            break;
        }
    }

    // Discard anything that didn't hit
    if ( dot( transmittance, vec3( 1 ) ) >= 3.0 ) {
        discard;
    }

    vec3 color = scattering;
    return color;
}


/*
==========================================
main
==========================================
*/
void main() {
    float time = timeTime.x;
   
    vec3 rayOrigin = camPos.xyz;
    vec3 rayDir = normalize( modelPos.xyz - camPos.xyz );

	vec3 color = RayMarch( rayOrigin, rayDir );

	outColor = vec4( color.rgb, 1.0 );
}