#pragma once

#include "core/Random.h"
#include "noise/SimplexNoise.h"

namespace voxel {
namespace noise {

template<class Volume>
void generate(Volume& volume, int octaves, float frequency, float persistence, core::Random& random) {
	const Region& region = volume.getRegion();
	Log::debug("Create new chunk at %i:%i:%i", region.getCentreX(), region.getCentreY(), region.getCentreZ());
	const int width = region.getWidthInVoxels();
	const int depth = region.getDepthInVoxels();
	const int lowerX = region.getLowerX();
	const int lowerZ = region.getLowerZ();

	const int noiseSeedOffsetX = random.random(0, 1000);
	const int noiseSeedOffsetZ = random.random(0, 1000);

	static constexpr Voxel grass = createVoxel(VoxelType::Grass1);
	static constexpr Voxel dirt = createVoxel(VoxelType::Dirt1);

	glm::vec2 p(noiseSeedOffsetX + lowerX, noiseSeedOffsetZ + lowerZ);
	for (int x = lowerX; x < lowerX + width; ++x, p.x += 1.0f) {
		for (int z = lowerZ; z < lowerZ + depth; ++z, p.y += 1.0f) {
			const float n = ::noise::Simplex::Noise2DClamped(p, octaves, persistence, frequency);
			const int ni = ::noise::norm(n) * (depth - 1);
			glm::ivec3 vp(x, 0, z);
			volume.setVoxel(vp, dirt);
			for (int y = 1; y < ni; ++y) {
				vp.y = y;
				volume.setVoxel(vp, grass);
			}
		}
	}
}

}
}