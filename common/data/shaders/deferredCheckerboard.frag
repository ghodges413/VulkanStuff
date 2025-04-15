#version 450

/*
==========================================
uniforms
==========================================
*

layout( binding = 3 ) uniform sampler2D texDiffuse;
layout( binding = 4 ) uniform sampler2D texNormal;
layout( binding = 5 ) uniform sampler2D texRoughness;
layout( binding = 6 ) uniform sampler2D texSpecular;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec4 worldPos;
layout( location = 2 ) in vec4 worldNormal;
layout( location = 3 ) in vec4 worldTangent;
layout( location = 4 ) flat in ivec4 modelids;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 FragData[ 3 ];



/*
==========================================
GetColorFromPositionAndNormal
==========================================
*/
vec3 GetColorFromPositionAndNormal( in vec3 worldPosition, in vec3 normal ) {
    const float pi = 3.141519;

    vec3 scaledPos = worldPosition.xyz * pi * 2.0;
    vec3 scaledPos2 = worldPosition.xyz * pi * 2.0 / 10.0 + vec3( pi / 4.0 );
    float s = cos( scaledPos2.x ) * cos( scaledPos2.y ) * cos( scaledPos2.z );  // [-1,1] range
    float t = cos( scaledPos.x ) * cos( scaledPos.y ) * cos( scaledPos.z );     // [-1,1] range

    vec3 colorMultiplier = vec3( 0.5, 0.5, 1 );
    if ( abs( normal.x ) > abs( normal.y ) && abs( normal.x ) > abs( normal.z ) ) {
        colorMultiplier = vec3( 1, 0.5, 0.5 );
    } else if ( abs( normal.y ) > abs( normal.x ) && abs( normal.y ) > abs( normal.z ) ) {
        colorMultiplier = vec3( 0.5, 1, 0.5 );
    }

    t = ceil( t * 0.9 );
    s = ( ceil( s * 0.9 ) + 3.0 ) * 0.25;
    vec3 colorB = vec3( 0.85, 0.85, 0.85 );
    vec3 colorA = vec3( 1, 1, 1 );
    vec3 finalColor = mix( colorA, colorB, t ) * s;

    return colorMultiplier * finalColor;
}

/*
 ==========================
 main
 ==========================
 */
void main() {
    // store position data
    FragData[ 0 ] = vec4( worldPos.xyz, 1.0 );

    // store diffuse color0
    vec3 diffuse = GetColorFromPositionAndNormal( worldPos.xyz, worldNormal.xyz );
    FragData[ 1 ] = vec4( diffuse, 1.0 );
    
    //
    // Transform the normal from tangent space to view space
    //
    vec3 norm   = normalize( worldNormal.xyz );
    
    // store the normal data
    FragData[ 2 ] = vec4( norm, 1.0 );
    FragData[ 2 ].a = float( modelids.x );
    
    // store roughness color
    FragData[ 0 ].a = 0;

    // store specular color
    FragData[ 1 ].a = 0;
}

