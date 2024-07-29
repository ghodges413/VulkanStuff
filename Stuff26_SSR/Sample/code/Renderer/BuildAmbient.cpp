//
//  BuildAmbient.cpp
//
#include "Renderer/Common.h"
#include "Renderer/BuildAmbient.h"
#include "Models/ModelManager.h"
#include "Models/ModelStatic.h"
#include "Graphics/Image.h"
#include "Graphics/Samplers.h"
#include "Graphics/AccelerationStructure.h"
#include "Graphics/Barrier.h"
#include "Math/Complex.h"
#include "Math/Random.h"
#include "Math/Morton.h"
#include "Math/Math.h"
#include "Miscellaneous/Fileio.h"

#include <assert.h>
#include <stdio.h>
#include <vector>


/*
====================================================
ClampF
====================================================
*/
static float ClampF( float val, float mins, float maxs ) {
	if ( val < mins ) {
		return mins;
	}
	if ( val > maxs ) {
		return maxs;
	}
	return val;
}

static const int g_numProbesX = AMBIENT_NODES_X;
static const int g_numProbesY = AMBIENT_NODES_Y;
static const int g_numProbesZ = AMBIENT_NODES_Z;


static void GetProbeCoord( int id, int & ix, int & iy, int & iz ) {
	// id = x + y * width + z * width * height
	ix = id % g_numProbesX;
	iy = ( id % ( g_numProbesX * g_numProbesY ) ) / g_numProbesX;
	iz = id / ( g_numProbesX * g_numProbesY );
}

static void TestProbeCoord() {
	printf( "Begin Test Probe Coord==========================\n" );
	for ( int i = 0; i < g_numProbesX * g_numProbesY * g_numProbesZ; i++ ) {
		int x;
		int y;
		int z;
		GetProbeCoord( i, x, y, z );

		printf( "id: %i   x: %i  y: %i  z: %i\n", i, x, y, z );
	}
	printf( "End Test Probe Coord==========================\n" );
}

static int g_probeCount = 0;

static int g_screenshotCount = 0;

static Vec3 g_cubeMap[ 256 * 256 * 6 ];

static const Bounds g_ambientBounds( Vec3( -35, -23, -1 ), Vec3( 20, 23, 11 ) );

legendrePolynomials_t * g_ambientNodes = NULL;

void SaveAmbientNodes() {
	const int maxProbes = g_numProbesX * g_numProbesY * g_numProbesZ;
	if ( maxProbes != g_probeCount ) {
		return;
	}
	g_probeCount++;

	ambientHeader_t header;
	header.magic = AMBIENT_MAGIC;
	header.version = AMBIENT_VERSION;
	header.numX = g_numProbesX;
	header.numY = g_numProbesY;
	header.numZ = g_numProbesZ;
	header.bounds = g_ambientBounds;

	char strName[ 256 ];
	sprintf_s( strName, 256, "generated/ambient.shambient" );

	OpenFileWriteStream( strName );
	WriteFileStream( &header, sizeof( ambientHeader_t ) );
	WriteFileStream( g_ambientNodes, maxProbes * sizeof( legendrePolynomials_t ) );
	CloseFileWriteStream();
	
 	//SaveFileData( strName, g_ambientNodes, maxProbes * sizeof( legendrePolynomials_t ) );
	free( g_ambientNodes );
	exit( 0 );
}




Vec3 GetAmbientPos() {
	//TestProbeCoord();
	int ix;
	int iy;
	int iz;
	GetProbeCoord( g_probeCount, ix, iy, iz );

	float dx = g_ambientBounds.WidthX() / float( g_numProbesX );
	float dy = g_ambientBounds.WidthY() / float( g_numProbesY );
	float dz = g_ambientBounds.WidthZ() / float( g_numProbesZ );

	float x = float( ix ) * dx + g_ambientBounds.mins.x;
	float y = float( iy ) * dy + g_ambientBounds.mins.y;
	float z = float( iz ) * dz + g_ambientBounds.mins.z;

	return Vec3( x, y, z );
}

Vec3 GetAmbientLookAt() {
	Vec3 pos = GetAmbientPos();
	
	Vec3 lookat = pos;
	if ( 0 == g_screenshotCount ) {
		lookat += Vec3( 1, 0, 0 );
	}
	if ( 1 == g_screenshotCount ) {
		lookat += Vec3(-1, 0, 0 );
	}
	if ( 2 == g_screenshotCount ) {
		lookat += Vec3( 0, 1, 0 );
	}
	if ( 3 == g_screenshotCount ) {
		lookat += Vec3( 0,-1, 0 );
	}
	if ( 4 == g_screenshotCount ) {
		lookat += Vec3( 0, 0, 1 );
	}
	if ( 5 == g_screenshotCount ) {
		lookat += Vec3( 0, 0,-1 );
	}

	return lookat;
}

/*
====================================================
BuildAmbient
====================================================
*/
void BuildAmbient( DeviceContext * device, FrameBuffer & frameBuffer ) {
	const int maxProbes = g_numProbesX * g_numProbesY * g_numProbesZ;
	if ( g_probeCount >= maxProbes ) {
		return;
	}

	if ( NULL == g_ambientNodes ) {
		g_ambientNodes = (legendrePolynomials_t *)malloc( sizeof( legendrePolynomials_t ) * maxProbes );
	}

	FrameBufferCaptureAmbient( device, frameBuffer );
	BuildAmbient();
	SaveAmbientNodes();
}

/*
====================================================
FrameBufferCaptureAmbient
====================================================
*/
void FrameBufferCaptureAmbient( DeviceContext * device, FrameBuffer & frameBuffer ) {
	if ( g_screenshotCount >= 6 ) {
		return;
	}

	VkResult result;

	bool screenshotSaved = false;
	bool supportsBlit = true;

	// Check blit support for source and destination
	VkFormatProperties formatProps;

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
	vkGetPhysicalDeviceFormatProperties( device->m_vkPhysicalDevice, frameBuffer.m_imageColor.m_parms.format, &formatProps );
	if ( !( formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) ) {
		//std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images 
	vkGetPhysicalDeviceFormatProperties( device->m_vkPhysicalDevice, frameBuffer.m_imageColor.m_parms.format, &formatProps );
	if ( !( formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) ) {
		//std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Source for the copy is the last rendered swapchain image
	//int currentBuffer = 0;
	//VkImage srcImage = m_vkColorImages[ currentBuffer ];
	VkImage srcImage = frameBuffer.m_imageColor.m_vkImage;
	
	// Create the linear tiled destination image to copy to and to read the memory from
	VkImageCreateInfo imageCreateCI = {};
	imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
	// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
	imageCreateCI.format = frameBuffer.m_imageColor.m_parms.format;
	imageCreateCI.extent.width = frameBuffer.m_parms.width;
	imageCreateCI.extent.height = frameBuffer.m_parms.height;
	imageCreateCI.extent.depth = 1;
	imageCreateCI.arrayLayers = 1;
	imageCreateCI.mipLevels = 1;
	imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// Create the image
	VkImage dstImage;
	result = vkCreateImage( device->m_vkDevice, &imageCreateCI, nullptr, &dstImage );
	if ( VK_SUCCESS != result ) {
		printf( "screenshot failed\n" );
		return;
	}

	// Create memory to back up the image
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo = {};//(vks::initializers::memoryAllocateInfo());
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkDeviceMemory dstImageMemory;
	vkGetImageMemoryRequirements( device->m_vkDevice, dstImage, &memRequirements );
	memAllocInfo.allocationSize = memRequirements.size;
	// Memory must be host visible to copy from
	memAllocInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
	
	result = vkAllocateMemory( device->m_vkDevice, &memAllocInfo, nullptr, &dstImageMemory );
	if ( VK_SUCCESS != result ) {
		printf( "screenshot failed\n" );
		return;
	}
	
	result = vkBindImageMemory( device->m_vkDevice, dstImage, dstImageMemory, 0 );
	if ( VK_SUCCESS != result ) {
		printf( "screenshot failed\n" );
		return;
	}

	// Do the actual blit from the swapchain image to our host visible destination image
	VkCommandBuffer copyCmd = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );
	
	// Transition destination image to transfer destination layout
	InsertImageMemoryBarrier(
		copyCmd,
		dstImage,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);

	// Transition swapchain image from present to transfer source layout
	InsertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);

	// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
	if ( supportsBlit ) {
		// Define the region to blit (we will blit the whole swapchain image)
		VkOffset3D blitSize;
		blitSize.x = frameBuffer.m_parms.width;
		blitSize.y = frameBuffer.m_parms.height;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		vkCmdBlitImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST
		);
	} else {
		// Otherwise use image copy (requires us to manually flip components)
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = frameBuffer.m_parms.width;
		imageCopyRegion.extent.height = frameBuffer.m_parms.height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		vkCmdCopyImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion
		);
	}

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on
	InsertImageMemoryBarrier(
		copyCmd,
		dstImage,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);

	// Transition back the swap chain image after the blit is done
	InsertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);
	
	device->FlushCommandBuffer( copyCmd, device->m_vkGraphicsQueue );

	// Get layout of the image (including row pitch)
	VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout( device->m_vkDevice, dstImage, &subResource, &subResourceLayout );

	// Map image memory so we can start copying from it
	const unsigned short * data;
	vkMapMemory( device->m_vkDevice, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data );
	data += subResourceLayout.offset;

	char strName[ 256 ];
	sprintf_s( strName, 256, "generated/screenshot%i.ppm", g_screenshotCount );


	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	bool colorSwizzle = false;

	// Check if source is BGR 
	// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
// 	if ( !supportsBlit ) {
// 		std::vector< VkFormat > formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
// 		colorSwizzle = ( std::find( formatsBGR.begin(), formatsBGR.end(), m_vkColorImageFormat ) != formatsBGR.end() );
// 	}

	const int w = device->m_swapChain.m_windowWidth;
	const int h = device->m_swapChain.m_windowHeight;

	char strTmp[ 64 ];
	sprintf_s( strTmp, 64, "%i %i\n", w, h );

	int cubemapIdx = g_screenshotCount;

// 	std::string fileStr;
// 	fileStr = "P3\n";
// 	fileStr += strTmp;
// 	fileStr += "255\n";
	for ( int i = 0; i < w * h; i++ ) {
		char tmpStr[ 64 ];
		unsigned short r = data[ i * 4 + 0 ];
		unsigned short g = data[ i * 4 + 1 ];
		unsigned short b = data[ i * 4 + 2 ];
		if ( colorSwizzle ) {
			std::swap( r, b );
		}
		float fr = Math::F16toF32( r );
		float fg = Math::F16toF32( g );
		float fb = Math::F16toF32( b );
		

		int pageOffset = cubemapIdx * 256 * 256;
		g_cubeMap[ pageOffset + i ].x = fr;
		g_cubeMap[ pageOffset + i ].y = fg;
		g_cubeMap[ pageOffset + i ].z = fb;

// 		int ir = (int)ClampF( fr * 255.0f, 0.0f, 255.0f );
// 		int ig = (int)ClampF( fg * 255.0f, 0.0f, 255.0f );
// 		int ib = (int)ClampF( fb * 255.0f, 0.0f, 255.0f );
// 		sprintf_s( tmpStr, 64, "%i %i %i\n", ir, ig, ib );
// 		fileStr += tmpStr;
	}
//	SaveFileData( strName, fileStr.c_str(), (unsigned int)fileStr.length() );

	// Clean up resources
	vkUnmapMemory( device->m_vkDevice, dstImageMemory );
	vkFreeMemory( device->m_vkDevice, dstImageMemory, nullptr );
	vkDestroyImage( device->m_vkDevice, dstImage, nullptr );

	screenshotSaved = true;

	g_screenshotCount++;
}

/*
====================================================
RemapRange01
====================================================
*/
static float RemapRange01( float val, float mins, float maxs ) {
	float delta = maxs - mins;
	float ab = val - mins;

	float t = ab / delta;

	if ( t < 0 ) {
		t = 0;
	}
	if ( t > 1 ) {
		t = 1;
	}

	return t;
}

/*
====================================================
CubeCoordToDir
====================================================
*/
static Vec3 CubeCoordToDir( int face, int u, int v ) {
	Vec3 dir = Vec3( 1, 0, 0 );

	const int width = 256;
	//const int height = 256;

	const int halfWidth = width / 2;
	//const int halfHeight = height / 2;

	const int s = u - halfWidth;	// [0,256)->[-128,128)
	const int t = v - halfWidth;	// [0,256)->[-128,128)

	if ( 0 == face ) {
		// +x page
		dir.x = halfWidth;
		dir.y = -s;
		dir.z = -t;
	}
	if ( 1 == face ) {
		// -x page
		dir.x = -halfWidth;
		dir.y = s;
		dir.z = -t;
	}
	if ( 2 == face ) {
		// +y page
		dir.x = s;
		dir.y = halfWidth;
		dir.z = -t;
	}
	if ( 3 == face ) {
		// -y page
		dir.x = -s;
		dir.y = -halfWidth;
		dir.z = -t;
	}

	if ( 4 == face ) {
		// +z page
		dir.x = t;
		dir.y = -s;
		dir.z = halfWidth;
	}
	if ( 5 == face ) {
		// -z page
		dir.x = -t;
		dir.y = -s;
		dir.z = -halfWidth;
	}

	return dir;
}


/*
====================================================
CubeCoordToDir2
====================================================
*/
static Vec3 CubeCoordToDir2( int face, int u, int v, int w, int h ) {
	float fu = float( u ) / float( w );
	float fv = float( v ) / float( h );

	int s = fu * 256;
	int t = fv * 256;

	return CubeCoordToDir( face, s, t );
}

/*
====================================================
PlaneRayIntersection
====================================================
*/
static Vec2 PlaneRayIntersection( Vec3 dir, Vec3 norm ) {
	float t = dir.Dot( norm );
	float scale = 1.0f / t;
	dir = dir * scale;

	Vec2 st;
	if ( norm.x > 0 ) {
		st = Vec2( dir.y, dir.z );
	}
	if ( norm.x < 0 ) {
		st = Vec2( dir.y, dir.z );
	}

	if ( norm.y > 0 ) {
		st = Vec2( dir.x, dir.z );
	}
	if ( norm.y < 0 ) {
		st = Vec2( dir.x, dir.z );
	}

	if ( norm.z > 0 ) {
		st = Vec2( dir.y, dir.x );
	}
	if ( norm.z < 0 ) {
		st = Vec2( dir.y, dir.x );
	}

	return st;
}

/*
====================================================
DirToCubeCoord
====================================================
*/
static void DirToCubeCoord( Vec3 dir, int & u, int & v, int & face ) {
	dir.Normalize();

	const int width = 256;
	const int height = 256;

	int pageIdx = 0;
// 	int u = 0;
// 	int v = 0;
	face = 0;
	u = 0;
	v = 0;

	if ( dir.x * dir.x >= dir.y * dir.y && dir.x * dir.x >= dir.z * dir.z ) {
		if ( dir.x > 0 ) {
			// +x page
			pageIdx = 0;

			Vec2 st = PlaneRayIntersection( dir, Vec3( 1, 0, 0 ) );
			float s = ( 1.0f - st.x ) * 0.5f;
			float t = ( 1.0f - st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		} else {
			// -x page
			pageIdx = 1;

			Vec2 st = PlaneRayIntersection( dir, Vec3( -1, 0, 0 ) );
			float s = ( 1.0f + st.x ) * 0.5f;
			float t = ( 1.0f - st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		}
	}

	if ( dir.y * dir.y >= dir.x * dir.x && dir.y * dir.y >= dir.z * dir.z ) {
		if ( dir.y > 0 ) {
			// +y page
			pageIdx = 2;

			Vec2 st = PlaneRayIntersection( dir, Vec3( 0, 1, 0 ) );
			float s = ( 1.0f + st.x ) * 0.5f;
			float t = ( 1.0f - st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		} else {
			// -y page
			pageIdx = 3;

			Vec2 st = PlaneRayIntersection( dir, Vec3( 0, -1, 0 ) );
			float s = ( 1.0f - st.x ) * 0.5f;
			float t = ( 1.0f - st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		}
	}

	if ( dir.z * dir.z >= dir.y * dir.y && dir.z * dir.z >= dir.x * dir.x ) {
		if ( dir.z > 0 ) {
			// +z page
			pageIdx = 4;

			Vec2 st = PlaneRayIntersection( dir, Vec3( 0, 0, 1 ) );
			float s = ( 1.0f - st.x ) * 0.5f;
			float t = ( 1.0f + st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		} else {
			// -z page
			pageIdx = 5;

			Vec2 st = PlaneRayIntersection( dir, Vec3( 0, 0, -1 ) );
			float s = ( 1.0f - st.x ) * 0.5f;
			float t = ( 1.0f - st.y ) * 0.5f;

			u = ( width - 1 ) * s;
			v = ( height - 1 ) * t;
		}
	}

	face = pageIdx;
}

/*
====================================================
SampleCubeMap
====================================================
*/
static Vec3 SampleCubeMap( Vec3 dir ) {
	const int width = 256;
	const int height = 256;

	int pageIdx = 0;
	int u = 0;
	int v = 0;
	DirToCubeCoord( dir, u, v, pageIdx );

	int pageOffset = width * height * pageIdx;
	int pixelIdx = u + v * width;
	Vec3 color = g_cubeMap[ pageOffset + pixelIdx ];
	return color;
}

/*
====================================================
TestCubeStuff
====================================================
*/
static void TestCubeStuff() {
	printf( "BEGIN Test CUBE ================================\n" );
	for ( int face = 0; face < 6; face++ ) {
		printf( "\n\nFace: %i\n", face );
		for ( int v = 1; v < 255; v += 60 ) {
			for ( int u = 1; u < 255; u += 60 ) {
				Vec3 dir = CubeCoordToDir( face, u, v );
				int u2 = 0;
				int v2 = 0;
				int face2 = 0;
				DirToCubeCoord( dir, u2, v2, face2 );

				printf( "u: %i v: %i f: %i  dir: %.2f %.2f %.2f   u: %i v: %i f: %i\n",
					u, v, face,
					dir.x, dir.y, dir.z,
					u2, v2, face2
				);
			}
		}
	}
	printf( "END Test CUBE ================================\n" );
}

/*
====================================================
BuildAmbient
====================================================
*/
void BuildAmbient() {
	// Wait until the cube map is full before calculating a probe
	if ( 6 != g_screenshotCount ) {
		return;
	}
	g_screenshotCount = 0;

	const int maxProbes = g_numProbesX * g_numProbesY * g_numProbesZ;
	printf( "Building Ambient Node:  %i  of  %i\n", g_probeCount, maxProbes );

	const float pi = acosf( -1 );

	legendrePolynomials_t * polyonmial = &g_ambientNodes[ g_probeCount ];
	memset( polyonmial, 0, sizeof( legendrePolynomials_t ) );

	// I am doing the lazy thing here and using a monte carlo integration to calculate the ylm's
	const int numSamples = 1024;
	const float dA = 4.0f * pi / (float)numSamples;
	for ( int n = 0; n < numSamples; n++ ) {
		Vec3 dir = Random::RandomOnSphereSurface();
		dir.Normalize();

		Vec3 color = SampleCubeMap( dir );

		const float x2 = dir[ 0 ] * dir[ 0 ];
		const float y2 = dir[ 1 ] * dir[ 1 ];
		const float z2 = dir[ 2 ] * dir[ 2 ];

		const float xz = dir[ 0 ] * dir[ 2 ];
		const float xy = dir[ 0 ] * dir[ 1 ];
		const float yz = dir[ 1 ] * dir[ 2 ];

		const float azimuth = sqrtf( 1.0f - dir[ 2 ] * dir[ 2 ] );

		Vec3 normalizedColorAzimuth = color * azimuth * dA;

		for ( int i = 0; i < 3; ++i ) {
			polyonmial->mL00[ i ] +=	0.282095f * normalizedColorAzimuth[ i ];
			polyonmial->mL11[ i ] +=	0.488603f * dir[ 0 ] * normalizedColorAzimuth[ i ];
			polyonmial->mL10[ i ] +=	0.488603f * dir[ 2 ] * normalizedColorAzimuth[ i ];
			polyonmial->mL1n1[ i ] +=	0.488603f * dir[ 1 ] * normalizedColorAzimuth[ i ];
			polyonmial->mL22[ i ] +=	0.546274f * ( x2 - y2 ) * normalizedColorAzimuth[ i ];
			polyonmial->mL21[ i ] +=	1.092548f * xz * normalizedColorAzimuth[ i ];
			polyonmial->mL20[ i ] +=	0.315392f * ( 3.0f * z2 - 1.0f ) * normalizedColorAzimuth[ i ];
			polyonmial->mL2n1[ i ] +=	1.092548f * yz * normalizedColorAzimuth[ i ];
			polyonmial->mL2n2[ i ] +=	1.092548f * xy * normalizedColorAzimuth[ i ];
		}			
	}
	
	g_probeCount++;
}
