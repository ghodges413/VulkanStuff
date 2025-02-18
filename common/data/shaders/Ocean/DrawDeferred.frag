#version 450
/*
==========================================
uniforms
==========================================
*/

layout( binding = 6 ) uniform uboFragment {
	vec4 camPos;
	vec4 dirToSun;
	vec4 camPos2;
} fragment;

layout( binding = 7 ) uniform sampler2D texNormals0;
layout( binding = 8 ) uniform sampler2D texNormals1;
layout( binding = 9 ) uniform sampler2D texNormals2;
layout( binding = 10 ) uniform sampler2D texNormals3;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec3	v_worldSpacPos;
layout( location = 1 ) in vec4	v_weights;
layout( location = 2 ) in vec2	v_texCoords[ 4 ];

/*
==========================================
output
==========================================
*/

//layout( location = 0 ) out vec4 outColor;
layout( location = 0 ) out vec4 FragData[ 3 ];



/*
 ==========================
 SnellsLaw
 L = vector to surface
 N = normal
 n1 = index of refraction
 n2 = index of refraction
 ==========================
 */
vec3 SnellsLaw( in vec3 L, in vec3 N, in float n1, in float n2 ) {
	float cosTheta1 = -dot( N, L );
	float r = n1 / n2;
	float cosTheta2 = sqrt( 1.0f - r * r * ( 1.0f - cosTheta1 * cosTheta1 ) );
	vec3 refract = r * L + ( r * cosTheta1 - cosTheta2 ) * N;
	return normalize( refract );
}

/*
 ==========================
 FresnelEquations
 L = vector to surface
 N = normal
 n1 = index of refraction
 n2 = index of refraction
 ==========================
 */
vec2 FresnelEquations( in vec3 L, in vec3 N, in float n1, in float n2 ) {
	float cosTheta1 = -dot( N, L );
	float r = n1 / n2;
	float cosTheta2 = sqrt( 1.0f - r * r * ( 1.0f - cosTheta1 * cosTheta1 ) );
	
	float Rs = ( n1 * cosTheta1 - n2 * cosTheta2 ) / ( n1 * cosTheta1 + n2 * cosTheta2 );
	Rs = Rs * Rs;
	
	float Rp = ( n1 * cosTheta2 - n2 * cosTheta1 ) / ( n1 * cosTheta2 + n2 * cosTheta1 );
	Rp = Rp * Rp;
	
	float R = ( Rs + Rp ) / 2.0f;
	float T = 1.0f - R;
	return vec2( R, T );
}

/*
==========================
GetSkyColor
==========================
*/
vec3 GetSkyColor( in vec3 e ) {
    e.z = max( e.z, 0.0f );
    vec3 ret;
    ret.r = pow( 1.0f - e.z, 2.0f );
    ret.g = 0.6f + ( 1.0f - e.z ) * 0.4f;
	ret.b = 1.0f - e.z;
	ret *= 0.015f;
	//ret = vec3( 0.3f, 0.73f, 0.63f );
	ret = vec3( 0, 0, 0 );
	// TODO: Generate a skybox and sample from that
    return ret;
}



/*
 ==========================
 main
 ==========================
 */
void main() {
	vec4 normals[ 4 ];
	normals[ 0 ] = texture( texNormals0, v_texCoords[ 0 ] ).xyzw;
	normals[ 1 ] = texture( texNormals1, v_texCoords[ 1 ] ).xyzw;
	normals[ 2 ] = texture( texNormals2, v_texCoords[ 2 ] ).xyzw;
	normals[ 3 ] = texture( texNormals3, v_texCoords[ 3 ] ).xyzw;
	vec4 normal = vec4( 0.0f );
	for ( int i = 0; i < 4; i++ ) {
		normal += normals[ i ] * v_weights[ i ];
	}
	//normal = vec4( 0, 0, 1, 0 );
	float dist = length( v_worldSpacPos.xyz );
	float t = dist / 1000.0f;
	t = 1.0f - exp( -t );
	normal = mix( normal, vec4( 0, 0, 1, 0 ), t );
	
	// Get the HDR color from the floating point diffuse buffer
	float Jacobian	= max( 0.0f, 0.5f - normals[ 0 ].w );
	float distToCam2D = length( fragment.camPos.xy - v_worldSpacPos.xy );
	Jacobian *= exp( -distToCam2D * 0.00075f );
	
	vec3 dirToCam = normalize( fragment.camPos.xyz - v_worldSpacPos );
	vec3 lookDir = -dirToCam;
	vec3 reflection = lookDir + 2.0f * normal.xyz;
	vec3 transmission = SnellsLaw( lookDir, normal.xyz, 1.0f, 1.33f );
	vec2 RT = FresnelEquations( lookDir, normal.xyz, 1.0f, 1.33f );
	float R = RT.x;
	float T = RT.y;
	
	vec3 skyColor = vec3( 0.0f, 0.0f, 1.0f );
	vec3 waterColor = vec3( 0.0f, 1.0f, 0.0f );
	
	skyColor = GetSkyColor( reflection );
	skyColor = GetSkyColor( lookDir );
	
	vec3 seaBase = vec3( 0.3f, 0.63f, 0.73f );
	if ( true ) {
		seaBase = vec3( 0.3f, 0.73f, 0.63f );
		seaBase *= 0.5f;
	} else {
		seaBase = vec3( 0.83f, 0.15f, 0.1f );
		seaBase *= 0.5f;
	}
	
	float tuner = 1.9f;
	waterColor = pow( seaBase, vec3( max( 0.0f, -transmission.z * tuner ) ) );// * 40.0f;
	waterColor = seaBase;

	float brightness = 0.0f;
	if ( -fragment.dirToSun.z > 0.0f ) {
		brightness = clamp( sqrt( -fragment.dirToSun.z ), 0.0f, 1.0f );
	}
	
	vec3 finalColor = skyColor * R + waterColor * T + Jacobian;

    vec3 worldSpacePos = v_worldSpacPos.xyz;

    // store position data
    FragData[ 0 ] = vec4( worldSpacePos, 1.0 );

    // store diffuse color0
    FragData[ 1 ] = vec4( waterColor.rgb, 1.0 );

    vec3 finalNorm = normal.xyz;
    
    // store the normal data
    FragData[ 2 ] = vec4( finalNorm, 1.0 );
    
    //Jacobian = 0;
    // store roughness color
	//float roughness = Jacobian;
	float roughness = 1.0;
    FragData[ 0 ].a = roughness;//texture( texRoughness, fragTexCoord ).r;

    // store metalness color
    float metalness = 1.0;// - Jacobian;
    FragData[ 1 ].a = metalness;//texture( texSpecular, fragTexCoord ).r;
}