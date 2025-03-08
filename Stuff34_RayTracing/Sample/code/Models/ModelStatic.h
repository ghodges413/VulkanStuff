//
//  model.h
//
#pragma once
#include <map>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include "Graphics/DeviceContext.h"
#include "Graphics/Buffer.h"
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Math/Bounds.h"


/*
====================================================
FloatToByte
// Assumes a float between [-1,1]
====================================================
*/
inline unsigned char FloatToByte_n11( const float f ) {
	int i = (int)(f * 127 + 128);
	return (unsigned char)i;
}
/*
====================================================
FloatToByte
// Assumes a float between [0,1]
====================================================
*/
inline unsigned char FloatToByte_01( const float f ) {
	int i = (int)(f * 255);
	return (unsigned char)i;
}

inline Vec3 Byte4ToVec3( const unsigned char * data ) {
	Vec3 pt;
	pt.x = float( data[ 0 ] ) / 255.0f;	// 0,1
	pt.y = float( data[ 1 ] ) / 255.0f;	// 0,1
	pt.z = float( data[ 2 ] ) / 255.0f;	// 0,1

	pt.x = 2.0f * ( pt.x - 0.5f ); //-1,1
	pt.y = 2.0f * ( pt.y - 0.5f ); //-1,1
	pt.z = 2.0f * ( pt.z - 0.5f ); //-1,1
	return pt;
}

inline void Vec3ToFloat3( const Vec3 & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
	f[ 2 ] = v.z;
}
inline void Vec2ToFloat2( const Vec2 & v, float * f ) {
	f[ 0 ] = v.x;
	f[ 1 ] = v.y;
}
inline void Vec3ToByte4( const Vec3 & v, unsigned char * b ) {
	Vec3 tmp = v;
	tmp.Normalize();
	b[ 0 ] = FloatToByte_n11( tmp.x );
	b[ 1 ] = FloatToByte_n11( tmp.y );
	b[ 2 ] = FloatToByte_n11( tmp.z );
	b[ 3 ] = 0;
}
inline void Vec3ToByte4_n11( unsigned char * a, const Vec3 & b ) {
	a[ 0 ] = FloatToByte_n11( b.x );
	a[ 1 ] = FloatToByte_n11( b.y );
	a[ 2 ] = FloatToByte_n11( b.z );
	a[ 3 ] = 0;
}

inline void Vec3ToByte4_01( unsigned char * a, const Vec3 & b ) {
	a[ 0 ] = FloatToByte_01( b.x );
	a[ 1 ] = FloatToByte_01( b.y );
	a[ 2 ] = FloatToByte_01( b.z );
	a[ 3 ] = 0;
}

struct tri_t {
	int a;
	int b;
	int c;

	bool operator == ( const tri_t & rhs ) const {
		if ( a != rhs.a && a != rhs.b && a != rhs.c ) {
			return false;
		}
		if ( b != rhs.a && b != rhs.b && b != rhs.c ) {
			return false;
		}
		if ( c != rhs.a && c != rhs.b && c != rhs.c ) {
			return false;
		}

		return true;
	}

	bool HasEdge( int A, int B ) const {
		if ( ( a == A && b == B ) || ( a == B && b == A ) ) {
			return true;
		}
		if ( ( b == A && c == B ) || ( b == B && c == A ) ) {
			return true;
		}
		if ( ( c == A && a == B ) || ( c == B && a == A ) ) {
			return true;
		}
		return false;			
	}

	int NumSharedEdges( const tri_t & rhs ) const {
		int num = 0;
		if ( rhs.HasEdge( a, b ) ) {
			num++;
		}
		if ( rhs.HasEdge( b, c ) ) {
			num++;
		}
		if ( rhs.HasEdge( c, a ) ) {
			num++;
		}
		return num;
	}

	bool SharesEdge( const tri_t & rhs ) const {
		return ( NumSharedEdges( rhs ) > 0 );
	}
};

/*
====================================================
vert_t
// 8 * 4 = 32 bytes - data structure for drawable verts... this should be good for most things
====================================================
*/
struct vert_t {
// 	float			xyz[ 3 ];	// 12 bytes
// 	float			st[ 2 ];	// 8 bytes
	Vec3 pos;
	Vec2 st;
	unsigned char	norm[ 4 ];	// 4 bytes
	unsigned char	tang[ 4 ];	// 4 bytes
	unsigned char	buff[ 4 ];	// 4 bytes

	static VkVertexInputBindingDescription GetBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vert_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array< VkVertexInputAttributeDescription, 5 > GetAttributeDescriptions() {
		std::array< VkVertexInputAttributeDescription, 5 > attributeDescriptions = {};

		attributeDescriptions[ 0 ].binding = 0;
		attributeDescriptions[ 0 ].location = 0;
		attributeDescriptions[ 0 ].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 0 ].offset = offsetof( vert_t, pos );

		attributeDescriptions[ 1 ].binding = 0;
		attributeDescriptions[ 1 ].location = 1;
		attributeDescriptions[ 1 ].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[ 1 ].offset = offsetof( vert_t, st );

		attributeDescriptions[ 2 ].binding = 0;
		attributeDescriptions[ 2 ].location = 2;
		attributeDescriptions[ 2 ].format = VK_FORMAT_R8G8B8A8_UNORM;
		attributeDescriptions[ 2 ].offset = offsetof( vert_t, norm );

		attributeDescriptions[ 3 ].binding = 0;
		attributeDescriptions[ 3 ].location = 3;
		attributeDescriptions[ 3 ].format = VK_FORMAT_R8G8B8A8_UNORM;
		attributeDescriptions[ 3 ].offset = offsetof( vert_t, tang );

		attributeDescriptions[ 4 ].binding = 0;
		attributeDescriptions[ 4 ].location = 4;
		attributeDescriptions[ 4 ].format = VK_FORMAT_R8G8B8A8_UNORM;
		attributeDescriptions[ 4 ].offset = offsetof( vert_t, buff );

		return attributeDescriptions;
	}

	static unsigned char FloatToByte_n11( const float f );
	static unsigned char FloatToByte_01( const float f );
	static Vec3 Byte4ToVec3( const unsigned char * data );
	static void Vec3ToFloat3( const Vec3 & v, float * f );
	static void Vec2ToFloat2( const Vec2 & v, float * f );
	static void Vec3ToByte4( const Vec3 & v, unsigned char * b );
};

class Shape;

/*
====================================================
Model
====================================================
*/
class Model {
public:
	Model() : m_isVBO( false ), m_device( NULL ) {}
	~Model() {}

	// These are the triangle soup vertices and indices
	std::vector< vert_t > m_vertices;
	std::vector< unsigned int > m_indices;

	struct Meshlet2_t {
		static const int maxVerts = 32;

		int vertOffset;
		int vertCount;
		int indexOffset;
		int indexCount;
	};
	struct MeshletBounds_t {
		Vec4 sphere;	// used for meshlet culling
		Vec4 cone;		// used for cone culling
	};

	// These are for a model that is a continuous mesh
	std::vector< vert_t > m_manifoldVertices;
	std::vector< unsigned int > m_manifoldIndices;
	std::vector< Meshlet2_t > m_manifoldMeshlets;
	std::vector< MeshletBounds_t > m_manifoldMeshletBounds;
	Buffer	m_manifoldVertexBuffer;
	Buffer	m_manifoldIndexBuffer;
	Buffer	m_manifoldMeshletBuffer;
	Buffer	m_manifoldBoundsBuffer;

	bool BuildFromShape( const Shape * shape );
	bool BuildFromCloth( float * verts, const int width, const int height, const int stride );
	void UpdateClothVerts( DeviceContext & deviceContext, float * verts, const int width, const int height, const int stride );

	bool LoadOBJ( const char * localFileName );
	void CalculateTangents();

	bool MakeVBO( DeviceContext * device );

	// GPU Data
	bool m_isVBO;
	Buffer	m_vertexBuffer;
	Buffer	m_indexBuffer;

	Bounds m_bounds;

	void Cleanup();

	void GetVerts( std::vector< Vec3 > & verts );

	void DrawIndexed( VkCommandBuffer vkCommandBUffer );

	void Decompose();
private:
	DeviceContext * m_device;	// the device the vbo was created with

	void BuildManifold();
	void BuildMeshletsBVH();
	

	friend class ModelManager;
};

void FillCube( Model & model );
void FillFullScreenQuad( Model & model );



struct RenderModel {
	RenderModel() : modelDraw( NULL ), subsurfaceId( -1 ), isTransparent( false ) {}

	Model * modelDraw;			// The vao buffer to draw
	uint32_t uboByteOffset;	// The byte offset into the uniform buffer
	uint32_t uboByteSize;	// how much space we consume in the uniform buffer

	uint32_t uboByteOffsetModelID;	// The byte offset into the uniform buffer
	uint32_t uboByteSizeModelID;	// how much space we consume in the uniform buffer

	int subsurfaceId;	// Used to enable/disable subsurface scattering for the model
						// TODO: This should probably be a property of the material

	bool isTransparent;
	Vec4 transparentColor;

	Vec3 pos;
	Quat orient;
};


bool OBJ_SaveSimple( const char * localFileName, const Vec3 * verts, const int numVerts, const int * indices, const int numIndices );