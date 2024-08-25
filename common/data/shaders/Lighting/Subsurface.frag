#version 450

/*
==========================================
Uniforms
==========================================
*

layout( binding = 0 ) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 2 ) uniform sampler2D texGbuffer0;
layout( binding = 3 ) uniform sampler2D texGbuffer1;
layout( binding = 4 ) uniform sampler2D texGbuffer2;

layout( binding = 5 ) uniform sampler2D texSubsurfaceTable;

/*
==========================================
input
==========================================
*/
//in vec4 gl_FragCoord;
layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec4 worldPos;
layout( location = 2 ) in vec4 worldNormal;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

/*
==========================================
CalculateInverseRadius
Calculating inverse radius prevents a divide by zero,
since the delta in position is guaranteed to not be zero (except at the edge of the screen)
==========================================
*/
#if 0
float CalculateInverseRadius( vec3 n1, vec3 dn, vec3 dx ) {
    //return 20;
    //float cosTheta = 1.0 + dot( n1, dn );
    vec3 n2 = n1 + dn;
    float cosTheta = dot( n2, n1 );

    float theta = acos( cosTheta );
    float invR = sin( theta * 0.5 ) / ( 0.5 * length( dx ) );
    if ( invR != invR ) {
        invR = 0;
    }
    return invR;
}
#else
float CalculateInverseRadius( vec3 dn, vec3 dx ) {
    float invR = ( length( dn ) / length( dx ) ) - 1.0;
    if ( invR < 0 ) {
        // Treat negative curvature as flat
        invR = 0;
    }
    return invR;
}
#endif

float InverseRadiusToCoord( float invR ) {
    float nearf = 0.005;
    float farf = 100.0;
    float invNear = 1.0 / nearf;
    float invFar = 1.0 / farf;
    float t = ( invR - invFar ) / ( invNear - invFar );
    return t;
}

/*
==========================================
main
==========================================
*/
void main() {
    vec2 st = fragTexCoord;
    vec4 gbuffer0 = texture( texGbuffer0, st );
    //vec4 gbuffer1 = texture( texGbuffer1, st );
    vec4 gbuffer2 = texture( texGbuffer2, st );

    //vec3 worldPos = gbuffer0.xyz;
    //vec3 diffuse = gbuffer1.rgb;
    //vec3 normal = gbuffer2.xyz;

    vec3 normal = normalize( worldNormal.xyz );
    normal = gbuffer2.xyz;

    // Calculate the local curvature of the fragment being rendered
    vec3 deltaPosX = dFdx( worldPos.xyz );
    vec3 deltaPosY = dFdy( worldPos.xyz );

    vec3 deltaNormX = dFdx( worldPos.xyz + normal.xyz );
    vec3 deltaNormY = dFdy( worldPos.xyz + normal.xyz );

    float invRadiusX = CalculateInverseRadius( deltaNormX, deltaPosX );
    float invRadiusY = CalculateInverseRadius( deltaNormY, deltaPosY );

    // TODO: Use the screen space direction of the light as a weight to determine which curvature to use
    float invRadius = max( invRadiusX, invRadiusY );
    float r = InverseRadiusToCoord( invRadius );

    float angle = ( normal.z + 1.0 ) * 0.5;
    vec2 lookup = vec2( angle, r );

    vec4 subsurface = texture( texSubsurfaceTable, lookup );
    outColor = subsurface * 0.105;
}