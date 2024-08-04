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
layout( binding = 6 ) uniform sampler2D texMask;

layout( binding = 7 ) uniform sampler2D texDepth;

/*
==========================================
input
==========================================
*/

//layout( location = 0 ) in vec2 fragTexCoord;
//layout( location = 1 ) in vec4 worldPos;
//layout( location = 2 ) in vec4 worldNormal;
//layout( location = 3 ) in vec4 worldTangent;

layout( location = 0 ) in vec4 fragTexCoord;
layout( location = 1 ) in mat4 cameraView;
layout( location = 5 ) in mat4 cameraProj;
layout( location = 9 ) in mat4 decalView;
layout( location = 13 ) in mat4 decalProj;


/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 FragData[ 3 ];

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
 ==========================
 main
 ==========================
 */
void main() {
    vec2 fragw = fragTexCoord.xy / fragTexCoord.w;
    vec2 st = fragTexCoord.xy / fragTexCoord.w;
    st = st * 0.5 + 0.5;

    float depth = texture( texDepth, st ).r;
    vec4 depthCoord = vec4( fragw.x, fragw.y, depth, 1.0 );

    mat4 invView = inverse( cameraView );
    mat4 invProj = inverse( cameraProj );

    vec4 worldPos = invView * invProj * depthCoord;
    worldPos.xyz /= worldPos.w;

    vec4 decalST = decalProj * decalView * vec4( worldPos.xyz, 1.0 );
    decalST.xyz /= decalST.w;

    if ( DoClip( decalST.xyz ) ) {
        discard;
    }

    decalST.xy = decalST.xy * 0.5 + 0.5;
    float mask = texture( texMask, decalST.xy ).r;
    if ( mask < 0.1 ) {
        discard;
    }

    // store position data
    FragData[ 0 ] = vec4( worldPos.xyz, 1.0 );
    //FragData[ 0 ] = vec4( 1.0, 0.0, 1.0, 1.0 );

    // store diffuse color0
    FragData[ 1 ] = texture( texDiffuse, decalST.xy );
    
    //
    // Transform the normal from tangent space to view space
    //
    //vec3 norm   = normalize( worldNormal.xyz );
    //vec3 tang   = normalize( worldTangent.xyz );
    //vec3 binorm = normalize( cross( norm, tang ) );

    // TODO: This is probably incorrect (will need to play with it to correct it)
    vec3 fwd = decalView[ 2 ].xyz;
    vec3 up = decalView[ 1 ].xyz;
    vec3 right = decalView[ 0 ].xyz;

    fwd = vec3( decalView[ 0 ].z, decalView[ 1 ].z, decalView[ 2 ].z );
    up = vec3( decalView[ 0 ].y, decalView[ 1 ].y, decalView[ 2 ].y );
    right = vec3( decalView[ 0 ].x, decalView[ 1 ].x, decalView[ 2 ].x );

    vec3 norm = fwd;
    vec3 tang = right;
    vec3 binorm = up;
    
    //vec3 normal = ( 2.0 * texture( texNormal, fragTexCoord ).rgb ) - 1.0;
    //normal      = normalize( normal );

    vec3 normal = texture( texNormal, decalST.xy ).rgb;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal = normalize( normal );

    vec3 finalNorm  = normal.x * tang + normal.y * binorm + normal.z * norm;
    finalNorm       = normalize( finalNorm );
    //finalNorm = normal;
    //finalNorm = norm;
    finalNorm = vec3( 0, 0, 1 );
    //finalNorm = vec3( 1, 0, 1 );
    
    // store the normal data
    FragData[ 2 ] = vec4( finalNorm, 1.0 );
    
    // store roughness color
    FragData[ 0 ].a = texture( texRoughness, decalST.xy ).r;

    // store specular color
    FragData[ 1 ].a = texture( texSpecular, decalST.xy ).r;
    
    // store the tang data
    //binorm  = normalize( cross( finalNorm, tang ) );
    //tang    = normalize( cross( binorm, finalNorm ) );
    //FragData[ 4 ] = vec4( tang, 1.0 );
}

