#include "Collection.hpp"
#include <string>

namespace libcube::jsystem {

#if 0
struct ExampleXF : public pl::TransformStack::XForm
{
	ExampleXF()
	{
		name = {
			"Triangle Stripping",
			"j3dcollection.tristrip",
			"tristrip"
		};
		addParam(
			pl::RichName{ "Cache Size", "cache_sz", "cache_size" },
			"1024"
			);
		addParam(
			pl::RichName{ "Minimum Strip Length", "min_strip", "minimum_strip_length" },
			"2"
		);
		addParam(
			pl::RichName{ "Push Cache Hits", "push_cache", "push_cache_hits" },
			true
		);
		addParam(pl::TransformStack::Param(
			{ "Algorithm", "algo", "algorithm" },
			{
				{ "SGI", "sgi", "sgi" },
				{ "MStrip", "mstrip", "mstrip" },
				{ "STRIPE", "stripe", "stripe"}
			},
			0
		));
	}
	void perform(const pl::TransformStack::XFormContext& ctx) const override
	{
		printf("Triangle stripping!\n");
		const auto cache_sz = std::stoi(ctx.getString("cache_sz"));
		const auto min_strip = std::stoi(ctx.getString("min_strip"));
		const auto push_cache = ctx.getFlag("push_cache");
		const auto algo = ctx.getEnumExposedCStr("algo");
		printf("cache_sz: %i, min_strip: %i, push_cache: %s, algo: %s\n", cache_sz, min_strip, push_cache ? "true" : "false", algo);
	}
};
#endif
const libcube::Texture& Material::getTexture(const std::string& id) const
{
	assert(mpScene);
	J3DCollection* j3dscene = dynamic_cast<J3DCollection*>(mpScene);
	assert(j3dscene);

	const auto textures = j3dscene->getTextures();

	for (int i = 0; i < textures.size(); ++i)
		if (textures[i].getName() == id)
			return textures[i];

	throw "Invalid";
}
} // namespace libcube::jsystem
