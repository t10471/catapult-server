/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "HashLockInfoCacheTypes.h"
#include "catapult/cache/BasicCache.h"

namespace catapult { namespace cache {

	/// Cache composed of hash lock info information.
	using BasicHashLockInfoCache = BasicCache<HashLockInfoCacheDescriptor, HashLockInfoCacheTypes::BaseSets>;

	/// Synchronized cache composed of hash lock info information.
	class HashLockInfoCache : public SynchronizedCache<BasicHashLockInfoCache> {
	public:
		DEFINE_CACHE_CONSTANTS(HashLockInfo)

	public:
		/// Creates a cache around \a config.
		explicit HashLockInfoCache(const CacheConfiguration& config)
				: SynchronizedCache<BasicHashLockInfoCache>(BasicHashLockInfoCache(config))
		{}
	};
}}