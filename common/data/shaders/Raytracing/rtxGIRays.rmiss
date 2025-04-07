#version 460
#extension GL_NV_ray_tracing : require

struct record_t {
	vec4 pos;
	vec4 norm;
	vec4 color;
};
layout( location = 0 ) rayPayloadInNV record_t hitValue;

/*
==========================================
main
==========================================
*/
void main() {
	vec3 sky = vec3( 0.5, 0.7, 1.0 );
	//vec3 sky = vec3( 0.0, 0.5, 1.0 );
    vec3 horizon = vec3( 1, 1, 1 );
	vec3 dir = normalize( gl_WorldRayDirectionNV );

    float t = abs( clamp( 0.5 + 0.5 * dir.z, 0.0, 1.0 ) );
    vec3 skycolor = mix( horizon, sky, t );
	
	hitValue.color = vec4( skycolor, -1.0 );	
}