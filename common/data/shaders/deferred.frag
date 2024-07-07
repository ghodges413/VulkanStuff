#version 450

/*
==========================================
uniforms
==========================================
*/

layout( binding = 2 ) uniform sampler2D texDiffuse;
layout( binding = 3 ) uniform sampler2D texNormal;
layout( binding = 4 ) uniform sampler2D texRoughness;
layout( binding = 5 ) uniform sampler2D texSpecular;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec4 worldPos;
layout( location = 2 ) in vec4 worldNormal;
layout( location = 3 ) in vec4 worldTangent;


/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 FragData[ 3 ];

/*
 ==========================
 main
 ==========================
 */
void main() {
    // store position data
    FragData[ 0 ] = vec4( worldPos.xyz, 1.0 );

    // store diffuse color0
    FragData[ 1 ] = texture( texDiffuse, fragTexCoord );
    
    //
    // Transform the normal from tangent space to view space
    //
    vec3 norm   = normalize( worldNormal.xyz );
    vec3 tang   = normalize( worldTangent.xyz );
    vec3 binorm = normalize( cross( norm, tang ) );
    
    //vec3 normal = ( 2.0 * texture( texNormal, fragTexCoord ).rgb ) - 1.0;
    //normal      = normalize( normal );

    vec3 normal = texture( texNormal, fragTexCoord ).rgb;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal = normalize( normal );

    vec3 finalNorm  = normal.x * tang + normal.y * binorm + normal.z * norm;
    finalNorm       = normalize( finalNorm );
    
    // store the normal data
    FragData[ 2 ] = vec4( finalNorm, 1.0 );
    
    // store roughness color
    FragData[ 0 ].a = texture( texRoughness, fragTexCoord ).r;

    // store specular color
    FragData[ 1 ].a = texture( texSpecular, fragTexCoord ).r;
    
    // store the tang data
    //binorm  = normalize( cross( finalNorm, tang ) );
    //tang    = normalize( cross( binorm, finalNorm ) );
    //FragData[ 4 ] = vec4( tang, 1.0 );
}

