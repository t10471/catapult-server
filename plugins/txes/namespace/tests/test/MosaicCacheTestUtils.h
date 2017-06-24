#pragma once
#include "src/cache/MosaicCacheStorage.h"
#include "src/cache/NamespaceCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache containing at least the mosaic cache.
	struct MosaicCacheFactory {
	private:
		static auto CreateSubCachesWithMosaicCache() {
			auto cacheId = cache::MosaicCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cache::NamespaceCache::Id] = MakeSubCachePlugin<cache::NamespaceCache, cache::NamespaceCacheStorage>();
			subCaches[cacheId] = MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			return cache::CatapultCache(CreateSubCachesWithMosaicCache());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration& config) {
			auto subCaches = CreateSubCachesWithMosaicCache();
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	/// Adds mosaic \a id to the mosaic cache in \a cache at \a height with \a duration and \a supply.
	void AddMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, ArtifactDuration duration, Amount supply);

	/// Adds mosaic \a id in namespace \a namespaceId to the mosaic cache in \a cache at \a height with \a duration
	/// and \a owner.
	void AddMosaic(
			cache::CatapultCacheDelta& cache,
			NamespaceId namespaceId,
			MosaicId id,
			Height height,
			ArtifactDuration duration,
			const Key& owner);

	/// Adds eternal mosaic \a id in namespace \a namespaceId to the mosaic cache in \a cache at \a height.
	void AddEternalMosaic(cache::CatapultCacheDelta& cache, NamespaceId namespaceId, MosaicId id, Height height);

	/// Adds eternal mosaic \a id in namespace \a namespaceId to the mosaic cache in \a cache at \a height with \a owner.
	void AddEternalMosaic(cache::CatapultCacheDelta& cache, NamespaceId namespaceId, MosaicId id, Height height, const Key& owner);

	/// Adds account \a owner to the account state cache in \a cache with \a amount units of mosaic \a id.
	void AddMosaicOwner(cache::CatapultCacheDelta& cache, MosaicId id, const Key& owner, Amount amount);

	/// Asserts that \a cache has the expected sizes (\a size and \a deepSize).
	void AssertCacheSizes(const cache::MosaicCacheView& cache, size_t size, size_t deepSize);

	/// Asserts that \a cache has the expected sizes (\a size and \a deepSize).
	void AssertCacheSizes(const cache::MosaicCacheDelta& cache, size_t size, size_t deepSize);

	/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCache& cache, std::initializer_list<MosaicId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCacheView& cache, std::initializer_list<MosaicId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCacheDelta& cache, std::initializer_list<MosaicId::ValueType> expectedIds);
}}
