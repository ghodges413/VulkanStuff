#version 450

/*
==========================================
uniforms
==========================================
*/

layout( binding = 2 ) uniform sampler2D texSampler;

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
main
==========================================
*/
void main() {
    outColor = texture( texSampler, fragTexCoord );
}