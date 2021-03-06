#pragma once

#include <memory>
#include <vector>

#include "entity.h"

namespace Halley {
	class EntityFactory;
	class ConfigFile;
	
	class EntityScene {
	public:
		std::vector<EntityRef>& getEntities();
		const std::vector<EntityRef>& getEntities() const;

		bool needsUpdate() const;
		void update(EntityFactory& factory);

		void addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity, std::optional<int> index = {});
		void addRootEntity(EntityRef entity);

	private:
		class PrefabObserver {
		public:
			PrefabObserver(std::shared_ptr<const ConfigFile> config);
			
			bool needsUpdate() const;
			bool isScene() const;
			
			void update(EntityFactory& factory);
			void markUpdated();

			void addEntity(EntityRef entity, std::optional<int> index);

			const std::shared_ptr<const ConfigFile>& getConfig() const;

		private:
			std::shared_ptr<const ConfigFile> config;
			std::vector<EntityRef> entities;
			int assetVersion = 0;
			bool scene = false;
		};

		std::vector<EntityRef> entities;
		std::vector<PrefabObserver> prefabObservers;

		PrefabObserver& getOrMakeObserver(const std::shared_ptr<const ConfigFile>& config);
	};
}
