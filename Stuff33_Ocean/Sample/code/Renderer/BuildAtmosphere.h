//
//  BuildAtmosphere.h
//
#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/Common.h"
#include "Graphics/DeviceContext.h"
#include "Math/Matrix.h"

class Buffer;
struct RenderModel;

struct atmosData_t {
	int transmittance_width = 64;
	int transmittance_height = 512;

	int irradiance_width = 16;
	int irradiance_height = 128;

	int scatter_mu_s_size = 32;
	int scatter_nu_size = 8;
	int scatter_mu_size = 128;
	int scatter_r_size = 32;
	Vec4 scatter_dims = Vec4( scatter_mu_s_size, scatter_nu_size, scatter_mu_size, scatter_r_size );

	int num_samples_extinction = 512;
	int num_samples_irradiance = 32;
	int num_samples_scatter = 64;
	int num_samples_scatter_spherical = 32;

	float average_ground_reflectance = 0.1f;
	float mie_g = 0.8f;

	// units of kilometers
	float radius_ground = 6372;
	float radius_top = 6472;

	// units of kilometers
	float scale_height_rayleigh = 7.994f;
	float scale_height_mie = 1.2f;

	Vec3 beta_rayleigh_scatter = Vec3( 0.0058f, 0.0135f, 0.0331f );
	Vec3 beta_rayleigh_extinction = Vec3( 0.0058f, 0.0135f, 0.0331f );

	Vec3 beta_mie_scatter = Vec3( 0.004f, 0.004f, 0.004f );
	Vec3 beta_mie_extinction = Vec3( 0.00444f, 0.00444f, 0.00444f ); // beta_mie_scatter / 0.9f
};

#define ATMOS_MAGIC 0x12345678
#define ATMOS_VERSION 0

struct atmosHeader_t {
	int magic;
	int version;
	atmosData_t data;
};

//void BuildAtmosTransmittanceTable( const atmosData_t & parms );
void BuildAtmosphereTables( const atmosData_t & data );

#if 0
// This is the data for an Earth like atmosphere
Atmosphere {
	NumSamples = 256	// Number of integration samples
	TextureResX = 32	// Height
	TextureResY = 128	// Sun Angle
	TextureResZ = 128	// View Angle

	//SunLightIntensity = ( 1.0 0.960784 0.949019 )
	SunLightIntensity = ( 1.0, 0.78132, 0.477507 )

	IndicesOfRefraction = ( 1.000271287 1.000274307 1.000275319 )  // indices of refraction

	MolecularDensity_NsR = 2.653e25
	MolecularDensity_NsM = 1.5e10
	
	RadiusGround = 6372.797	// the radius of the earth [km]
	RadiusTop = 6472.797		// the radius of the top of the atmosphere [km]
	
	ScaleHeightRayleigh = 7.994	// This is in kilometers Earth
	ScaleHeightMie = 1.2				// This is in kilometers Earth
	
	BetaRayleighScatter = ( 0.0058 0.0135 0.0331 )		//5.8*10-3,1.35*10-2,3.31*10-2 Earth
	BetaRayleighExtinction = ( 0.0058, 0.0135, 0.0331 )
	
	BetaMieScatter = ( 0.004 0.004 0.004 )					//4.0*10-3, 4.0*10-3, 4.0*10-3 Earth
	BetaMieExtinction = ( 0.00444 0.00444 0.00444 )		//mBetaMieScatter / 0.9f;
	
	Planet = "Earth"
}


// This is the data for a Martian like atmosphere
Atmosphere {
	NumSamples = 64	// Number of integration samples
	TextureResX = 8	// Height
	TextureResY = 64	// Sun Angle
	TextureResZ = 64	// View Angle

	//SunLightIntensity = ( 1.0 0.960784 0.949019 )
	SunLightIntensity = ( 1.0, 0.78132, 0.477507 )

	IndicesOfRefraction = ( 1.000271287 1.000274307 1.000275319 )  // indices of refraction

	MolecularDensity_NsR = 2.653e25
	MolecularDensity_NsM = 1.5e10
	
	RadiusGround = 3389.5	// the radius of the mars [km]
	RadiusTop = 3589.5		// the radius of the top of the atmosphere [km]
	
	ScaleHeightRayleigh = 11.0	// This is in kilometers
	ScaleHeightMie = 1.2				// This is in kilometers
	
	BetaRayleighScatter = ( 0.019918 0.01357 0.00575 )
	BetaRayleighExtinction = ( 0.019918 0.01357 0.00575 )
	
	BetaMieScatter = ( 0.004 0.004 0.004 )					//4.0*10-3, 4.0*10-3, 4.0*10-3 Earth
	BetaMieExtinction = ( 0.00444 0.00444 0.00444 )		//mBetaMieScatter / 0.9f;
	
	Planet = "Mars"
}
#endif