#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ShaderImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Shader; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
