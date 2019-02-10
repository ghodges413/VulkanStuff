//
//  model.h
//
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

/*
====================================================
vert_t
// 8 * 4 = 32 bytes - data structure for drawable verts... this should be good for most things
====================================================
*/
struct vert_t {
	float			xyz[ 3 ];	// 12 bytes
	float			st[ 2 ];	// 8 bytes
	unsigned char	norm[ 4 ];	// 4 bytes
	unsigned char	tang[ 4 ];	// 4 bytes
	unsigned char	buff[ 4 ];	// 4 bytes
};

/*
====================================================
Model
====================================================
*/
class Model {
public:
	Model() {}
	~Model() {}

	std::vector< vert_t > m_vertices;
	std::vector< unsigned int > m_indices;

	bool LoadOBJ( const char * localFileName );
	void CalculateTangents();
};

void FillCube( Model & model );
void FillFullScreenQuad( Model & model );