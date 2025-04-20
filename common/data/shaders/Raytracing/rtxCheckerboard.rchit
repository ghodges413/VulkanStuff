
#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#if 1
struct vert_t {
	vec3 pos;
	vec2 st;
	uint norm;
	uint tang;
	uint buff;
};
#else
// Doing things this way didn't work either
struct vert_t {
	float x;
	float y;
	float z;
	float s;
	float t;
	float norm;
	float tang;
	float buff;
};
#endif
//layout( binding = 9, set = 0 ) readonly buffer VertexArray { float v[]; } Vertices[];
//layout( binding = 9, set = 0 ) readonly buffer VertexArray { vert_t v[]; } Vertices[];
//layout( binding = 10, set = 0 ) readonly buffer IndexArray { uint i[]; } Indices[];
//layout( binding = 11, set = 0 ) readonly buffer OrientArray { mat4 Orients[]; };

#define USE_TRIANGLES
#ifdef USE_TRIANGLES
//#define USE_VERTS_STRUCT
#ifdef USE_VERTS_STRUCT
// unfortunately using a struct causes padding issues
// the std430 is meant to more tightly pack things, but it's still not good enough
// check for more info: https://registry.khronos.org/OpenGL/extensions/ARB/ARB_shader_storage_buffer_object.txt
layout( std430, binding = 3, set = 0 ) readonly buffer VertexArray { vert_t v[]; } Vertices[];
#else
layout( binding = 3, set = 0 ) readonly buffer VertexArray { float v[]; } Vertices[];
#endif
layout( binding = 4, set = 0 ) readonly buffer IndexArray { uint i[]; } Indices[];
layout( binding = 5, set = 0 ) readonly buffer OrientArray { mat4 Orients[]; };
#endif

struct record_t {
	vec4 pos;
	vec4 norm;
	vec4 color;
};
layout( location = 0 ) rayPayloadInNV record_t hitValue;

hitAttributeNV vec3 attribs;	// the barycentric coordinates of the hit triangle

/*
==========================================
UnpackNormal
==========================================
*/
vec3 UnpackNormal( in uint n ) {
	//unpackUnorm2x16
	//unpackSnorm2x16
	//unpackUnorm4x8
	//unpackSnorm4x8
	//vec4 norm = unpackSnorm4x8( n );
	vec4 norm = unpackUnorm4x8( n );
	norm = norm * 2.0 - 1.0;
	return norm.xyz;
}

/*
==========================================
UnpackColor
==========================================
*/
vec4 UnpackColor( in uint c ) {
	//unpackUnorm2x16
	//unpackSnorm2x16
	//unpackUnorm4x8
	//unpackSnorm4x8
	vec4 color = unpackUnorm4x8( c );
	return color;
}

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

    //vec3 colorMultiplier = vec3( 0.5, 0.5, 1 );
	vec3 colorMultiplier = vec3( 0, 0, 1 );
    if ( abs( normal.x ) > abs( normal.y ) && abs( normal.x ) > abs( normal.z ) ) {
        colorMultiplier = vec3( 1, 0.5, 0.5 );
		colorMultiplier = vec3( 1, 0, 0 );
    } else if ( abs( normal.y ) > abs( normal.x ) && abs( normal.y ) > abs( normal.z ) ) {
        colorMultiplier = vec3( 0.5, 1, 0.5 );
		colorMultiplier = vec3( 0, 1, 0 );
    }

    t = ceil( t * 0.9 );
    s = ( ceil( s * 0.9 ) + 3.0 ) * 0.25;
    vec3 colorB = vec3( 0.85, 0.85, 0.85 );
    vec3 colorA = vec3( 1, 1, 1 );
    vec3 finalColor = mix( colorA, colorB, t ) * s;

    return colorMultiplier * finalColor;
}

/*
==========================================
GetRecord
==========================================
*/
#ifdef USE_TRIANGLES
record_t GetRecord( in uint iID, in uint primitiveID ) {
	// gl_InstanceID = model id
	// gl_PrimtiveID = triangle id
#ifdef USE_VERTS_STRUCT
	uint a = Indices[ iID ].i[ 3 * primitiveID + 0 ];
	uint b = Indices[ iID ].i[ 3 * primitiveID + 1 ];
	uint c = Indices[ iID ].i[ 3 * primitiveID + 2 ];
	
	uint stride = 1;
	vert_t v0 = Vertices[ iID ].v[ stride * a ];
	vert_t v1 = Vertices[ iID ].v[ stride * b ];
	vert_t v2 = Vertices[ iID ].v[ stride * c ];
/*
	vec3 n0 = UnpackNormal( v0.norm );
	vec3 n1 = UnpackNormal( v1.norm );
	vec3 n2 = UnpackNormal( v2.norm );

	vec4 c0 = UnpackColor( v0.buff );
	vec4 c1 = UnpackColor( v1.buff );
	vec4 c2 = UnpackColor( v2.buff );
*/	
	vec3 uvw;
	uvw.x = 1.0 - attribs.x - attribs.y;
	uvw.y = attribs.x;
	uvw.z = attribs.y;

	vec3 pos0 = vec3( v0.x, v0.y, v0.z );
	vec3 pos1 = vec3( v1.x, v1.y, v1.z );
	vec3 pos2 = vec3( v1.x, v2.y, v2.z );
	
	//vec3 pos = v0.pos * uvw.x + v1.pos * uvw.y + v2.pos * uvw.z;
	vec3 pos = pos0 * uvw.x + pos1 * uvw.y + pos2 * uvw.z;
//	vec3 norm = n0 * uvw.x + n1 * uvw.y + n2 * uvw.z;
//	vec2 st = v0.st * uvw.x + v1.st * uvw.y + v2.st * uvw.z;
//	vec4 color = c0 * uvw.x + c1 * uvw.y + c2 * uvw.z;
	//color.rgb = vec3( 0, 1, 0 );
/*	
	float x2 = norm.x * norm.x;
	float y2 = norm.y * norm.y;
	float z2 = norm.z * norm.z;
	if ( x2 > y2 && x2 > z2 ) {
		color.rgb = vec3( 1, 0, 0 );
	}
	if ( y2 > z2 && y2 > x2 ) {
		color.rgb = vec3( 0, 1, 0 );
	}
	if ( z2 > x2 && z2 > y2 ) {
		color.rgb = vec3( 0, 0, 1 );
	}
*/
	//norm = n0;
	vec3 norm = vec3( 0, 0, 1 );
	vec3 ab = pos1 - pos0;
	vec3 bc = pos2 - pos1;
	norm = cross( ab, bc );
	norm = normalize( norm );
	vec4 color;
	color.rgb = GetColorFromPositionAndNormal( pos, norm );
	//color = c0;
	color.rgb = abs( norm );
	//color.rgb = pos;
#else
	uint a = Indices[ iID ].i[ 3 * primitiveID + 0 ];
	uint b = Indices[ iID ].i[ 3 * primitiveID + 1 ];
	uint c = Indices[ iID ].i[ 3 * primitiveID + 2 ];
	
	uint stride = 8; // sizeof( vert_t ) / sizeof( float )
	vec3 v0 = vec3( Vertices[ iID ].v[ stride * a + 0 ], Vertices[ iID ].v[ stride * a + 1 ], Vertices[ iID ].v[ stride * a + 2 ] );
	vec3 v1 = vec3( Vertices[ iID ].v[ stride * b + 0 ], Vertices[ iID ].v[ stride * b + 1 ], Vertices[ iID ].v[ stride * b + 2 ] );
	vec3 v2 = vec3( Vertices[ iID ].v[ stride * c + 0 ], Vertices[ iID ].v[ stride * c + 1 ], Vertices[ iID ].v[ stride * c + 2 ] );

	vec2 st0 = vec2( Vertices[ iID ].v[ stride * a + 3 + 0 ], Vertices[ iID ].v[ stride * a + 3 + 1 ] );
	vec2 st1 = vec2( Vertices[ iID ].v[ stride * b + 3 + 0 ], Vertices[ iID ].v[ stride * b + 3 + 1 ] );
	vec2 st2 = vec2( Vertices[ iID ].v[ stride * c + 3 + 0 ], Vertices[ iID ].v[ stride * c + 3 + 1 ] );

	vec3 n0 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * a + 5 ] ) );
	vec3 n1 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * b + 5 ] ) );
	vec3 n2 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * c + 5 ] ) );

	vec3 t0 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * a + 6 ] ) );
	vec3 t1 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * b + 6 ] ) );
	vec3 t2 = UnpackNormal( floatBitsToUint( Vertices[ iID ].v[ stride * c + 6 ] ) );

	vec4 c0 = UnpackColor( floatBitsToUint( Vertices[ iID ].v[ stride * a + 7 ] ) );
	vec4 c1 = UnpackColor( floatBitsToUint( Vertices[ iID ].v[ stride * b + 7 ] ) );
	vec4 c2 = UnpackColor( floatBitsToUint( Vertices[ iID ].v[ stride * c + 7 ] ) );

	vec3 uvw;
	uvw.x = 1.0 - attribs.x - attribs.y;
	uvw.y = attribs.x;
	uvw.z = attribs.y;

	vec3 pos = v0 * uvw.x + v1 * uvw.y + v2 * uvw.z;
	vec2 st = st0 * uvw.x + st1 * uvw.y + st2 * uvw.z;
	vec3 norm = n0 * uvw.x + n1 * uvw.y + n2 * uvw.z;
	vec3 tang = t0 * uvw.x + t1 * uvw.y + t2 * uvw.z;
	vec4 color = c0 * uvw.x + c1 * uvw.y + c2 * uvw.z;

	mat4 orient = Orients[ iID ];
	vec4 pos2 = orient * vec4( pos, 1.0 );
	vec4 norm2 = orient * vec4( norm, 0.0 );
	pos = pos2.xyz;
	norm = norm2.xyz;

	vec3 ab = v1 - v0;
	vec3 bc = v2 - v1;
//	norm = normalize( cross( ab, bc ) );

	color.rgb = GetColorFromPositionAndNormal( pos, norm );
//	vec4 color = vec4( 1 );
//	color.rgb = abs( norm );
#endif
	record_t record;
	record.pos = vec4( pos, 1.0 );
	record.norm = vec4( norm, 0.0 );
	record.color = color;
	return record;
}
#endif

/*
==========================================
main
==========================================
*/
void main() {
	//gl_WorldRayOriginNV,
	//gl_WorldRayDirectionNV,
	//gl_ObjectRayOriginNV,
	//gl_ObjectRayDirectionNV
	
	//gl_RayTminNV
	//gl_RayTmaxNV = gl_HitTNV
	
	//gl_ObjectToWorldNV	// matrix for the current instance
	//gl_WorldToObjectNV	// matrix for the current instance
	
	vec3 pos;
	pos = gl_WorldRayOriginNV + gl_RayTmaxNV * gl_WorldRayDirectionNV;
	pos = gl_ObjectRayOriginNV + gl_RayTmaxNV * gl_ObjectRayDirectionNV;
	hitValue.pos.xyz = pos;
	hitValue.color = vec4( 1 );

#ifdef USE_TRIANGLES
	//gl_InstanceID		// The index of the model instance we hit
	//gl_PrimitiveID	// The index of the triangle in the model we hit
	hitValue = GetRecord( gl_InstanceID, gl_PrimitiveID );

	//mat4 orient = Orients[ gl_InstanceID ];
	//hitValue.pos = orient * hitValue.pos;
	//hitValue.norm = orient * hitValue.norm;
#else
	pos = gl_WorldRayOriginNV + gl_RayTmaxNV * gl_WorldRayDirectionNV;
	hitValue.pos.xyz = pos;
	hitValue.norm.xyz = gl_WorldRayDirectionNV * -1.0;
#endif
	//hitValue.color = vec4( 0, 1, 0, 1 );
}



