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

#include "src/observers/Observers.h"
#include "tests/test/LockInfoCacheTestUtils.h"
#include "tests/test/LockNotificationsTestUtils.h"
#include "tests/test/LockObserverTests.h"

namespace catapult { namespace observers {

#define TEST_CLASS HashLockObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(HashLock,)

	namespace {
		struct HashObserverTraits {
		public:
			using CacheType = cache::HashLockInfoCache;
			using NotificationType = model::HashLockNotification;
			using NotificationBuilder = test::HashLockNotificationBuilder;
			using ObserverTestContext = test::ObserverTestContextT<test::HashLockInfoCacheFactory>;

			static auto CreateObserver() {
				return CreateHashLockObserver();
			}

			static auto GenerateRandomLockInfo(const NotificationType& notification) {
				auto lockInfo = test::BasicHashLockInfoTestTraits::CreateLockInfo();
				lockInfo.Hash = notification.Hash;
				return lockInfo;
			}

			static const auto& ToKey(const NotificationType& notification) {
				return notification.Hash;
			}

			static void AssertAddedLockInfo(const model::HashLockInfo& lockInfo, const NotificationType& notification) {
				// Assert:
				EXPECT_EQ(notification.Hash, lockInfo.Hash);
			}
		};
	}

	DEFINE_LOCK_OBSERVER_TESTS(HashObserverTraits)
}}
