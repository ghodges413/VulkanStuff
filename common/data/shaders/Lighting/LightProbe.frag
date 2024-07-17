#version 450

/*
==========================================
Samplers
==========================================
*/

layout( binding = 1 ) uniform samplerCube texLightProbe;

layout( binding = 2 ) uniform sampler2D texGbuffer0;
layout( binding = 3 ) uniform sampler2D texGbuffer1;
layout( binding = 4 ) uniform sampler2D texGbuffer2;

/*
==========================================
input
==========================================
*/

layout( location = 0 ) in vec2 fragTexCoord;
layout( location = 1 ) in vec3 cameraPos;

/*
==========================================
output
==========================================
*/

layout( location = 0 ) out vec4 outColor;

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
    float specular = gbuffer1.a;
    //roughness = 0.15;
    //specular = 0.5;  // uncomment and adjust to brute force test the lightprobe
    
    vec3 dirToCamera = normalize( cameraPos - worldPos.xyz );

    // Get the reflection vector for the center of the lobe (which should move towards the normal as the lobe increases in size)
    vec3 ray = dirToCamera * -1.0;
    vec3 ref = reflect( ray, normal );
    ref = mix( ref, normal, roughness );
    ref = normalize( ref );

    // Sample the light/reflection probe by the shininess of the material
    vec4 probeColor = textureLod( texLightProbe, ref, roughness * 8.0 ) * specular;
    outColor = probeColor;
}