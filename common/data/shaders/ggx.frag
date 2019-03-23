#version 450

/*
==========================================
uniforms
==========================================
*/

layout( binding = 2 ) uniform sampler2D texDiffuse;
layout( binding = 3 ) uniform sampler2D texGlossiness;
layout( binding = 4 ) uniform sampler2D texNormal;
layout( binding = 5 ) uniform sampler2D texSpecular;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec3 cameraPos;
layout( location = 2 ) in vec4 worldPos;
layout( location = 3 ) in vec4 worldNormal;
layout( location = 4 ) in vec4 worldTangent;

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
    float F = FresnelShlick( N, V, F0 );
//    float F = FresnelShlick( L, H, F0 );
	
	// V - Smith approximation of the bidirectional shadowing masking
	float visibility =
            GeometricSelfShadowing( N, L, alpha ) *
            GeometricSelfShadowing( N, V, alpha );
	
	float specular = dotNL * D * F * visibility;
	return max( specular, 0.0 );
}

/*
==========================================
main
==========================================
*/
void main() {
    vec3 dirToLight = normalize( vec3( 1, 1, 1 ) );
    vec3 dirToCamera = normalize( cameraPos - worldPos.xyz );

    vec3 diffuse = texture( texDiffuse, fragTexCoord ).rgb;
    vec3 gloss = texture( texGlossiness, fragTexCoord ).rgb;
    vec3 normal = texture( texNormal, fragTexCoord ).xyz;
    vec3 specular = texture( texSpecular, fragTexCoord ).rgb;

    vec3 roughness = vec3( 1, 1, 1 ) - gloss;

    // normal needs to be transformed from texture space to world space
    normal.x = 2.0 * ( normal.x - 0.5 );
    normal.y = 2.0 * ( normal.y - 0.5 );

    vec3 worldBitangent = cross( worldNormal.xyz, worldTangent.xyz );    // this might be backwards
//    vec3 worldBitangent = cross( worldTangent.xyz, worldNormal.xyz );    // this might be backwards
    normal = normal.x * worldTangent.xyz + normal.y * worldBitangent.xyz + normal.z * worldNormal.xyz;
    normal = worldNormal.xyz;

    //vec3 specular;
    specular.r = GGX( normal, dirToCamera, dirToLight, roughness.r, specular.r );
    specular.g = GGX( normal, dirToCamera, dirToLight, roughness.g, specular.g );
    specular.b = GGX( normal, dirToCamera, dirToLight, roughness.b, specular.b );

    float ambient = 0.0;
    float flux = clamp( dot( normal, dirToLight ), 0.0, 1.0 ) + ambient;

    vec4 finalColor;
    finalColor.r = diffuse.r * flux + specular.r;
    finalColor.g = diffuse.g * flux + specular.g;
    finalColor.b = diffuse.b * flux + specular.b;
    finalColor.a = 1.0;

    outColor = finalColor;
//    outColor.rgb = diffuse.rgb * clamp( dot( worldNormal.xyz, dirToLight.xyz ), 0.0, 1.0 );
    outColor.rgb = worldNormal.xyz;
    //outColor.rgb = normal.xyz;
//    outColor.rgb = worldTangent.xyz;
    //outColor.rgb = dirToCamera.xyz;
}