#version 450

/*
==========================================
Uniforms
==========================================
*/

layout( push_constant ) uniform bounds {
    vec4 mins;
    vec4 maxs;
} PushConstant;

/*
==========================================
Samplers
==========================================
*/

layout( binding = 0 ) uniform sampler3D texAmbientRed;
layout( binding = 1 ) uniform sampler3D texAmbientGreen;
layout( binding = 2 ) uniform sampler3D texAmbientBlue;

layout( binding = 3 ) uniform sampler2D texGbuffer0;
layout( binding = 4 ) uniform sampler2D texGbuffer1;
layout( binding = 5 ) uniform sampler2D texGbuffer2;

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
ColorFromSH
==========================================
*/
vec3 ColorFromSH( vec3 dir, in vec3 ylm00, in vec3 ylm11, in vec3 ylm10, in vec3 ylm1n1 ) {
    // These ylm's were calculated from my linked program on my github
    //vec3 ylm00 = vec3( 0.285016, 0.285016, 0.209534 );
    //vec3 ylm11 = vec3( 0.194225, -0.001365, 0.000291 );
    //vec3 ylm10 = vec3( -0.001073, -0.001073, -0.105487 );
    //vec3 ylm1n1 = vec3( -0.003512, -0.196372, 0.193152 );
    //vec3 ylm22 = vec3( 0 );
    //vec3 ylm21 = vec3( 0 );
    //vec3 ylm20 = vec3( 0 );
    //vec3 ylm2n1 = vec3( 0 );
    //vec3 ylm2n2 = vec3( 0 );

    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;

    vec3 color = ylm00 * c4
        + ( ylm11 * dir.x + ylm1n1 * dir.y + ylm10 * dir.z ) * 2.0 * c2;
        //- ylm20 * c5
        //+ ylm20 * c3 * dir.z * dir.z
        //+ ylm22 * c1 * ( dir.x * dir.x - dir.y * dir.y )
        //+ ( ylm2n2 * dir.x * dir.y + ylm21 * dir.x * dir.z + ylm2n1 * dir.y * dir.z ) * 2.0 * c1;
    return color;
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

    // These mins/maxs are the bounds of the 3d texture holding the nodes
    vec3 mins = PushConstant.mins.xyz;
    vec3 maxs = PushConstant.maxs.xyz;

    // Convert the world space position to uvw coords of the 3d textures
    vec3 dims = maxs - mins;
    vec3 samplePos = worldPos - mins;
    vec3 uvw = samplePos / dims;

    // Sample textures
    vec4 red = texture( texAmbientRed, uvw );
    vec4 green = texture( texAmbientGreen, uvw );
    vec4 blue = texture( texAmbientBlue, uvw );

    // Convert to spherical harmonic components
    vec3 ylm00 = vec3( red.x, green.x, blue.x );
    vec3 ylm11 = vec3( red.y, green.y, blue.y );
    vec3 ylm10 = vec3( red.z, green.z, blue.z );
    vec3 ylm1n1 = vec3( red.w, green.w, blue.w );

    vec3 ambientColor = ColorFromSH( normal, ylm00, ylm11, ylm10, ylm1n1 );

    outColor.rgb = ambientColor * diffuse;
    outColor.a = 1.0;
}