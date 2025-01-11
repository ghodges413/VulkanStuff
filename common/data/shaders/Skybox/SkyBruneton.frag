#version 450

/*
==========================================
Uniforms
==========================================
*/
layout( binding = 1 ) uniform uboSky {
    vec4 camera;
    vec4 sun;   // xyz = dir, w = intensity
    // float sunIntensity; // packed into above w component
    mat4 shadowView;
    mat4 shadowProj;

    vec4 dimScatter;// = vec4( 32, 128, 32, 8 );

    vec4 radii; // ground, top, shadowNear, shadowFar
    //float radiusGround;
    //float radiusTop;
    //float shadowNear;
    //flaot shadowFar;

    // Rayleigh
    vec4 betaRayleighScatter;
    //float scaleHeightRayleigh;
    vec4 betaRayleighExtinction;

    // Mie
    vec4 betaMieScatter;
    //float scaleHeightMie;
    vec4 betaMieExtinction;
    //float mieG;
} skyparms;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 2 ) uniform sampler2D transmittanceTable;
layout( binding = 3 ) uniform sampler2D irradianceTable;    // precomputed skylight irradiance (E table)
layout( binding = 4 ) uniform sampler3D scatterTable;       // precomputed inscattered light (S table)

layout( binding = 5 ) uniform sampler2D texGbuffer0;
layout( binding = 6 ) uniform sampler2D texGbuffer1;
layout( binding = 7 ) uniform sampler2D texGbuffer2;

layout( binding = 8 ) uniform sampler2D texShadow;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec3 ray;
layout( location = 2 ) in mat4 v_viewInverse;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
===============================
texture4D
===============================
*/
vec4 texture4D( in sampler3D table, in float radius, in float cosAngleView, in float cosAngleSun, in float cosAngleViewSun ) {
    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

    float H = sqrt( radiusTop * radiusTop - radiusGround * radiusGround );
    float rho = sqrt( radius * radius - radiusGround * radiusGround );

    // I'm curious if we have mu_s and mu mixed up here...
    // TODO: re-read the paper to figure it out
    float scatter_mu_s_size = skyparms.dimScatter.x;
	float scatter_nu_size = skyparms.dimScatter.y;
	float scatter_mu_size = skyparms.dimScatter.z;
	float scatter_r_size = skyparms.dimScatter.w;

	float rmu = radius * cosAngleView;
	float delta = rmu * rmu - radius * radius + radiusGround * radiusGround;
	
	vec4 cst = vec4( -1.0, H * H, H, 0.5 + 0.5 / scatter_mu_s_size );
	if ( rmu < 0.0 && delta > 0.0 ) {
		cst = vec4( 1.0, 0.0, 0.0, 0.5 - 0.5 / scatter_mu_s_size );
	}
	
	float uR = 0.5 / scatter_r_size + rho / H * ( 1.0 - 1.0 / scatter_r_size );
	float uMu = cst.w + ( rmu * cst.x + sqrt( delta + cst.y ) ) / ( rho + cst.z ) * ( 0.5 - 1.0 / scatter_mu_s_size );
	
	// paper formula
	float uMuS = 0.5 / scatter_mu_size + max( ( 1.0 - exp( -3.0 * cosAngleSun - 0.6 ) ) / ( 1.0 - exp( -3.6 ) ), 0.0 ) * ( 1.0 - 1.0 / scatter_mu_size );
    
	float t = ( cosAngleViewSun + 1.0 ) / 2.0 * ( scatter_nu_size - 1.0 );
    float uNu = floor( t );
    t = t - uNu;
	
	vec3 coord0 = vec3( ( uNu + uMuS ) / scatter_nu_size, uMu, uR );
	vec3 coord1 = vec3( ( uNu + uMuS + 1.0 ) / scatter_nu_size, uMu, uR );
	
	vec4 table0 = texture( table, coord0 );
	vec4 table1 = texture( table, coord1 );
    return mix( table0, table1, t );
}

/*
===============================
Transmittance
// Returns the calculated transmittance for the height/angle.  Ignores ground intersections.
===============================
*/
vec3 Transmittance( in float radius, in float cosAngle ) {
    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

	vec2 st;
	st.x = ( radius - radiusGround ) / ( radiusTop - radiusGround );
    st.y = 0.5 * ( cosAngle + 1.0 );
    return texture( transmittanceTable, st ).rgb;
}

/*
===============================
TransmittanceWithShadow
// Transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
===============================
*/
vec3 TransmittanceWithShadow( in float radius, in float cosAngle ) {
    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

    if ( cosAngle < -sqrt( 1.0 - ( radiusGround / radius ) * ( radiusGround / radius ) ) ) {
		return vec3( 0.0 );
	}

	return Transmittance( radius, cosAngle );
}

/*
===============================
Transmittance
// Transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// r=||x||, mu=cos(zenith angle of [x,x0) ray at x), v=unit direction vector of [x,x0) ray
===============================
*/
vec3 Transmittance( in float radius, in float cosAngle, in vec3 v, in vec3 x0 ) {
    vec3 result;
    float r1 = length( x0 );
    float mu1 = dot( x0, v ) / radius;
    if ( cosAngle > 0.0 ) {
        result = min( Transmittance( radius, cosAngle ) / Transmittance( r1, mu1 ), 1.0 );
    } else {
        result = min( Transmittance( r1, -mu1 ) / Transmittance( radius, -cosAngle ), 1.0 );
    }
    return result;
}

/*
===============================
Transmittance
// Transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
===============================
*/
vec3 Transmittance( in float radius, in float cosAngle, in float d ) {
    vec3 result;
    float r1 = sqrt( radius * radius + d * d + 2.0 * radius * cosAngle * d );
    float mu1 = ( radius * cosAngle + d ) / r1;
    if ( cosAngle > 0.0 ) {
        result = min( Transmittance( radius, cosAngle ) / Transmittance( r1, mu1 ), 1.0 );
    } else {
        result = min( Transmittance( r1, -mu1 ) / Transmittance( radius, -cosAngle ), 1.0 );
    }
    return result;
}

/*
===============================
Irradiance
===============================
*/
vec3 Irradiance( in float radius, in float cosAngle ) {
    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

	vec2 st;
	st.x = ( radius - radiusGround ) / ( radiusTop - radiusGround );
    st.y = 0.5f * ( cosAngle + 1.0f );
    
    return texture( irradianceTable, st ).rgb;
}

/*
==========================
ScatterPhaseFunctionRayleigh
// Equation 2 from Bruneton2008
==========================
*/
float ScatterPhaseFunctionRayleigh( in float cosTheta ) {
	const float pi = acos( -1.0 );

	float phase = ( 3.0 / ( 16.0 * pi ) ) * ( 1.0 + cosTheta * cosTheta );
	return phase;
}

/*
==========================
ScatterPhaseFunctionMie
// Equation 4 from Bruneton2008
==========================
*/
float ScatterPhaseFunctionMie( in float cosTheta ) {
	const float pi = acos( -1.0 );

    float mieG = skyparms.betaMieExtinction.w;

	float g = mieG;
	float g2 = g * g;
	
	float phase = ( 3.0 / ( 8.0 * pi ) ) * ( 1.0 - g2 ) * ( 1.0 + cosTheta * cosTheta ) / ( ( 2.0 + g2 ) * pow( (1.0 + g2 - 2.0 * g * cosTheta ), 1.5 ) );
	return phase;
}

/*
===============================
ApproximateMie
// algorithm described in Bruneton2008 Section4 paragraph Angular Precision
===============================
*/
vec3 ApproximateMie( in vec4 inscatter ) {
	// Grab C* from the paper
	vec3 inscatterRayleigh = inscatter.rgb;
	
	// Grab Cm from the paper
	float inscatterMie = inscatter.a;
	
	// Calculate the Cmr / C*r (and don't divide by zero)
	float Cr = max( inscatterRayleigh.r, 1e-6 );
	float inscatterRatio = inscatterMie / Cr;
	
	// Calculate beta ratio from the paper (mie cancels itself out... so it's only rayleigh)
	vec3 betaRayleighRatio = skyparms.betaRayleighScatter.r / skyparms.betaRayleighScatter.rgb;
	
	// Output the final equation
	return inscatterRayleigh * inscatterRatio * betaRayleighRatio;
}

/*
===============================
Inscatter
//inscattered light along ray x+tv, when sun in direction s (=S[L])
===============================
*/
vec3 Inscatter( inout vec3 x, vec3 v, vec3 s, out float r, out float mu ) {
    float sunIntensity = skyparms.sun.w;

    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

    vec3 result;
    r = length( x );
    mu = dot( x, v ) / r;
    float d = -r * mu - sqrt( r * r * ( mu * mu - 1.0 ) + radiusTop * radiusTop );
    if ( d > 0.0 ) {
		// if x in space and ray intersects atmosphere
        // move x to nearest intersection of ray with top atmosphere boundary
        x += d * v;
        mu = ( r * mu + d ) / radiusTop;
        r = radiusTop;
    }
    if ( r <= radiusTop ) { // if ray intersects atmosphere
        float nu = dot( v, s );
        float muS = dot( x, s ) / r;
        float phaseR = ScatterPhaseFunctionRayleigh( nu );
        float phaseM = ScatterPhaseFunctionMie( nu );
        vec4 inscatter = max( texture4D( scatterTable, r, mu, muS, nu ), 0.0 );
        result = max( inscatter.rgb * phaseR + ApproximateMie( inscatter ) * phaseM, 0.0 );
    } else { // x in space and ray looking in space
        result = vec3( 0.0 );
    }
    return result * sunIntensity;
}

/*
===============================
GroundColor
//ground radiance at end of ray x+tv, when sun in direction s
//attenuated bewteen ground and viewer (=R[L0]+R[L*])
===============================
*/
vec3 GroundColor( vec3 x, vec3 v, vec3 s, float r, float mu ) {
	const float pi = acos( -1.0 );

    float sunIntensity = skyparms.sun.w;

    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

    vec3 result;
    float d = -r * mu - sqrt( r * r * ( mu * mu - 1.0 ) + radiusGround * radiusGround);
    if ( d > 0.0 ) { // if ray hits ground surface
        // ground reflectance at end of ray, x0
        vec3 x0 = x + d * v;
        vec3 n = normalize( x0 );
        vec2 coords = vec2( atan( n.y, n.x ), acos( n.z ) ) * vec2( 0.5, 1.0 ) / pi + vec2( 0.5, 0.0 );
        //vec4 reflectance = texture( planetSampler, coords ) * vec4( 0.2, 0.2, 0.2, 1.0 );
        vec4 reflectance = vec4( 0.2, 0.2, 0.2, 1.0 );

        // direct sun light (radiance) reaching x0
        float muS = dot( n, s );
        vec3 sunLight = TransmittanceWithShadow( radiusGround, muS );

        // precomputed sky light (irradiance) (=E[L*]) at x0
        vec3 groundSkyLight = Irradiance( radiusGround, muS );

        // light reflected at x0 (=(R[L0]+R[L*])/T(x,x0))
        vec3 groundColor = reflectance.rgb * ( max( muS, 0.0 ) * sunLight + groundSkyLight ) * sunIntensity / pi;

        // attenuation of light to the viewer, T(x,x0)
        vec3 attenuation = Transmittance( r, mu, v, x0 );

        // water specular color due to sunLight
        if ( reflectance.w > 0.0 ) {
            vec3 h = normalize( s - v );
            float fresnel = 0.02 + 0.98 * pow( 1.0 - dot( -v, h ), 5.0 );
            float waterBrdf = fresnel * pow( max( dot( h, n ), 0.0 ), 150.0 );
            groundColor += reflectance.w * max( waterBrdf, 0.0 ) * sunLight * sunIntensity;
        }

        result = attenuation * groundColor; //=R[L0]+R[L*]
    } else { // ray looking at the sky
        result = vec3( 0.0 );
    }
    return result;
}


/*
===============================
GroundColorGeo
//ground radiance at end of ray x+tv, when sun in direction s
//attenuated bewteen ground and viewer (=R[L0]+R[L*])
===============================
*/
vec3 GroundColorGeo( vec3 x, vec3 v, vec3 s, float r, float mu, vec3 normal, float shadowFactor ) {
	const float pi = acos( -1.0 );

    float sunIntensity = skyparms.sun.w;
    sunIntensity = min( sunIntensity, 2.0 );    // HACK: TODO: remove this when we have proper hdr/tonemapping/exposure parameters

    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

	// ground reflectance at end of ray, x0
	vec3 x0 = x;// + d * v;
	vec3 n = normalize( x );
	vec2 coords = vec2( atan( n.y, n.x ), acos( n.z ) ) * vec2( 0.5, 1.0 ) / pi + vec2( 0.5, 0.0 );

	// direct sun light (radiance) reaching x0
	float muS = dot( n, s );
	float muS2 = dot( normal, s );
	vec3 sunLight = TransmittanceWithShadow( radiusGround, muS ) * shadowFactor;

	// precomputed sky light (irradiance) (=E[L*]) at x0
	vec3 groundSkyLight = Irradiance( radiusGround, muS );
	groundSkyLight *= ( normal.z * 0.5 + 0.5 );	// The full of hemisphere of blue light is proportional to how "up" the surface points

	// light reflected at x0 (=(R[L0]+R[L*])/T(x,x0))
	vec3 groundColor = ( max( muS2, 0.0 ) * sunLight + groundSkyLight ) * sunIntensity / pi;

	// TODO: attenuation of light to the viewer, T(x,x0)
	vec3 attenuation = vec3( 1.0, 1.0, 1.0 );//Transmittance( r, mu, v, x0 );

	vec3 result = attenuation * groundColor; //=R[L0]+R[L*]
	
    return result;
}

/*
 ===============================
 SunColor
 // direct sun light for ray x+tv, when sun in direction s (=L0)
 ===============================
 */
vec3 SunColor( vec3 x, vec3 v, vec3 s, float r, float mu ) {
	const float pi = acos( -1.0 );

    float sunIntensity = skyparms.sun.w;

    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

    vec3 transmittance = vec3( 1.0 );
    if ( r <= radiusTop ) {
        transmittance = TransmittanceWithShadow( r, mu ); // T(x,xo)
    }
    float sun = step( cos( pi / 180.0 ), dot( v, s ) ) * sunIntensity; // Lsun
    return transmittance * sun; // Eq (9)
}

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand( in vec2 co ) {
    return fract( sin( dot( co, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );
}

/*
===========================
LightGroundGeometry
===========================
*/
void LightGroundGeometry( in vec3 worldPos, in vec3 normal, in vec3 albedo ) {
    float radiusGround = skyparms.radii.x;
    float radiusTop = skyparms.radii.y;

	vec3 x = skyparms.camera.xyz;
    vec3 v = normalize( ray );
    vec3 s = skyparms.sun.xyz;
	float r = radiusGround;
	float mu = 0.0;

    //
    //  Calculate Shadow Factor
    //
    mat4 shadowProj = skyparms.shadowProj;
    mat4 shadowView = skyparms.shadowView;
    float near = skyparms.radii.z;
    float far = skyparms.radii.w;

    vec4 shadowPos = shadowProj * shadowView * vec4( worldPos, 1.0 );
    shadowPos.xyz /= shadowPos.w;
    
    vec3 shadowST = shadowPos.xyz * 0.5 + 0.5;
    float depth = texture( texShadow, shadowST.xy ).r;

    float delta = far - near;
    float invDelta = 1.0 / delta;
    float bias = 0.04;

    float shadowFactor = 0.0;
    if ( depth - shadowPos.z > bias * invDelta ) {
        shadowFactor = 0.2; // not in shadow
    }    

    // Sample the shadow using offsets to smooth out jaggies
    vec2 shadowOffsets[ 4 ];
    float sizeFactor = 1.1;
    float dim = 2048;//PushConstant.parms.x;
    shadowOffsets[ 0 ] = vec2( 1, 1 ) * sizeFactor / dim;
    shadowOffsets[ 1 ] = vec2(-1, 1 ) * sizeFactor / dim;
    shadowOffsets[ 2 ] = vec2(-1,-1 ) * sizeFactor / dim;
    shadowOffsets[ 3 ] = vec2( 1,-1 ) * sizeFactor / dim;

    // Use a random rotation for sampling to make the artifacting noisy (this is less offensive to the eye)
    float theta = rand( shadowST.xy ) * 2.0 * 3.1415;// + PushConstant.parms.y;
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
	
    //
    //  Get the ground color
    //
	vec3 groundColor = GroundColorGeo( x, v, s, r, mu, normal.xyz, shadowFactor ); //R[L0]+R[L*]
	
	outColor.rgb = groundColor;
	outColor.rgb *= albedo.rgb;
}

/*
===============================
main
===============================
*/
void main() {
    vec2 st = fragTexCoord.xy;

    vec4 gbuffer0 = texture( texGbuffer0, st );
    vec4 gbuffer1 = texture( texGbuffer1, st );
    vec4 gbuffer2 = texture( texGbuffer2, st );

    vec3 worldPos = gbuffer0.xyz;
    vec3 diffuse = gbuffer1.rgb;
    vec3 normal = gbuffer2.xyz;
	
    if ( gbuffer2.a > 0 ) {
		LightGroundGeometry( worldPos, normal, diffuse );
	} else {
		vec3 x = skyparms.camera.xyz;
		vec3 v = normalize( ray );
        vec3 s = skyparms.sun.xyz;
	
		float r, mu;    // mu is the angle we're looking
		vec3 inscatterColor = Inscatter( x, v, s, r, mu ); //S[L]-T(x,xs)S[l]|xs = S[L] for spherical ground
		vec3 groundColor = GroundColor( x, v, s, r, mu ); //R[L0]+R[L*]
		vec3 sunColor = SunColor( x, v, s, r, mu ); //L0

		outColor = vec4( sunColor + groundColor + inscatterColor, 1.0 ); // Eq (16)
	}
    outColor.a = 1.0;
}
