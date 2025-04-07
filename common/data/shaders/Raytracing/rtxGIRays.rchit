
#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct vert_t {
	vec3 pos;
	vec2 st;
	uint norm;
	uint tang;
	uint buff;
};

//layout( binding = 9, set = 0 ) readonly buffer VertexArray { float v[]; } Vertices[];
//layout( binding = 9, set = 0 ) readonly buffer VertexArray { vert_t v[]; } Vertices[];
//layout( binding = 10, set = 0 ) readonly buffer IndexArray { uint i[]; } Indices[];
//layout( binding = 11, set = 0 ) readonly buffer OrientArray { mat4 Orients[]; };

#define USE_TRIANGLES
#ifdef USE_TRIANGLES
#define USE_VERTS_STRUCT
#ifdef USE_VERTS_STRUCT
layout( binding = 9, set = 0 ) readonly buffer VertexArray { vert_t v[]; } Vertices[];
#else
layout( binding = 9, set = 0 ) readonly buffer VertexArray { float v[]; } Vertices[];
#endif
layout( binding = 10, set = 0 ) readonly buffer IndexArray { uint i[]; } Indices[];
layout( binding = 11, set = 0 ) readonly buffer OrientArray { mat4 Orients[]; };
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
	vec4 norm = unpackSnorm4x8( n );
	vec3 nn;
	nn = norm.xyz;
	return nn;
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
GetRecord
==========================================
*/
#ifdef USE_TRIANGLES
record_t GetRecord( in uint iID, in uint primitiveID ) {
#ifdef USE_VERTS_STRUCT
	uint a = Indices[ iID ].i[ 3 * primitiveID + 0 ];
	uint b = Indices[ iID ].i[ 3 * primitiveID + 1 ];
	uint c = Indices[ iID ].i[ 3 * primitiveID + 2 ];
	
	uint stride = 1;
	vert_t v0 = Vertices[ iID ].v[ stride * a ];
	vert_t v1 = Vertices[ iID ].v[ stride * b ];
	vert_t v2 = Vertices[ iID ].v[ stride * c ];

	vec3 n0 = UnpackNormal( v0.norm );
	vec3 n1 = UnpackNormal( v1.norm );
	vec3 n2 = UnpackNormal( v2.norm );

	vec4 c0 = UnpackColor( v0.buff );
	vec4 c1 = UnpackColor( v1.buff );
	vec4 c2 = UnpackColor( v2.buff );
	
	vec3 uvw;
	uvw.x = 1.0 - attribs.x - attribs.y;
	uvw.y = attribs.x;
	uvw.z = attribs.y;
	
	vec3 pos = v0.pos * uvw.x + v1.pos * uvw.y + v2.pos * uvw.z;
	vec3 norm = n0 * uvw.x + n1 * uvw.y + n2 * uvw.z;
	vec2 st = v0.st * uvw.x + v1.st * uvw.y + v2.st * uvw.z;
	vec4 color = c0 * uvw.x + c1 * uvw.y + c2 * uvw.z;
#else
	uint a = Indices[ iID ].i[ 3 * primitiveID + 0 ];
	uint b = Indices[ iID ].i[ 3 * primitiveID + 1 ];
	uint c = Indices[ iID ].i[ 3 * primitiveID + 2 ];
	
	uint stride = 3;
	vec3 v0 = vec3( Vertices[ iID ].v[ stride * a + 0 ], Vertices[ iID ].v[ stride * a + 1 ], Vertices[ iID ].v[ stride * a + 2 ] );
	vec3 v1 = vec3( Vertices[ iID ].v[ stride * b + 0 ], Vertices[ iID ].v[ stride * b + 1 ], Vertices[ iID ].v[ stride * b + 2 ] );
	vec3 v2 = vec3( Vertices[ iID ].v[ stride * c + 0 ], Vertices[ iID ].v[ stride * c + 1 ], Vertices[ iID ].v[ stride * c + 2 ] );

	vec3 uvw;
	uvw.x = 1.0 - attribs.x - attribs.y;
	uvw.y = attribs.x;
	uvw.z = attribs.y;

	vec3 pos = v0 * uvw.x + v1 * uvw.y + v2 * uvw.z;

	vec3 ab = v1 - v0;
	vec3 bc = v2 - v1;
	vec3 norm = normalize( cross( ab, bc ) );

	vec4 color = vec4( 1 );
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
	hitValue.color = vec4( 0 );

#ifdef USE_TRIANGLES
	//gl_InstanceID		// The index of the model instance we hit
	//gl_PrimitiveID	// The index of the triangle in the model we hit
	hitValue = GetRecord( gl_InstanceID, gl_PrimitiveID );

	mat4 orient = Orients[ gl_InstanceID ];

	hitValue.pos = orient * hitValue.pos;
	hitValue.norm = orient * hitValue.norm;
#endif
}



