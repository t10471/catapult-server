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

#include "catapult/cache_db/RdbTypedColumnContainer.h"
#include "tests/test/nodeps/ParamsCapture.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS RdbTypedColumnContainerTests

	namespace {
		struct DummyValue {
		public:
			std::string KeyCopy;
			int Integer;
			double FloatingPoint;
		};

		template<typename TData>
		auto MutatePointer(const TData* data) {
			return 0x7FFFFFFF'FFFFFFFFull + reinterpret_cast<const uint8_t*>(data);
		}

		size_t MutateSize(size_t size) {
			return 123456 + size;
		}

		struct ColumnDescriptor {
		public:
			using KeyType = std::string;
			using ValueType = DummyValue;
			using StorageType = std::pair<KeyType, ValueType>;

			static const KeyType& GetKeyFromElement(const StorageType& element) {
				return element.first;
			}

			static const KeyType& GetKeyFromValue(const ValueType& value) {
				return value.KeyCopy;
			}

			struct Serializer {
			public:
				static RawBuffer SerializeKey(const KeyType& key) {
					// mutate the pointer and size to verify that serialized data is forwarded to container
					return { MutatePointer(key.data()), MutateSize(key.size()) };
				}

				static std::string SerializeValue(const StorageType& element) {
					std::ostringstream out;
					const auto& value = element.second;
					out << value.Integer << " " << std::fixed << std::setprecision(2) << value.FloatingPoint;
					return out.str();
				}

				static ValueType DeserializeValue(const RawBuffer&) {
					return { "world", 54321, 2.718281 };
				}
			};
		};

		struct InsertParamsType {
		public:
			InsertParamsType(const RawBuffer& key, const std::string& value)
					: Key(key)
					, Value(value)
			{}

		public:
			RawBuffer Key;
			std::string Value;
		};

		struct FindParamsType {
		public:
			FindParamsType(const RawBuffer& key, RdbDataIterator& iterator)
					: Key(key)
					, pIterator(&iterator)
			{}

		public:
			RawBuffer Key;
			RdbDataIterator* pIterator;
		};

		struct RemoveParamsType {
		public:
			RemoveParamsType(const RawBuffer& key) : Key(key)
			{}

		public:
			RawBuffer Key;
		};

		struct MockDb {
		public:
			explicit MockDb(bool isKeyFound = false) : IsKeyFound(isKeyFound)
			{}

		public:
			auto size() const {
				return Size;
			}

			void find(const RawBuffer& key, RdbDataIterator& iterator) {
				FindParams.push(key, iterator);
				iterator.setFound(IsKeyFound);
			}

		private:
			bool IsKeyFound;

		public:
			size_t Size = 0;
			size_t SavedSize = 0;

			test::ParamsCapture<InsertParamsType> InsertParams;
			test::ParamsCapture<FindParamsType> FindParams;
			test::ParamsCapture<RemoveParamsType> RemoveParams;
		};

		// mock replacing RdbColumnContainer
		struct MockContainer {
		public:
			MockContainer(MockDb& db, size_t) : m_db(db)
			{}

		public:
			auto size() const {
				return m_db.size();
			}

			void saveSize(size_t newSize) {
				m_db.SavedSize = newSize;
			}

			bool insert(const RawBuffer& key, const std::string& value) {
				m_db.InsertParams.push(key, value);
				return true;
			}

			bool find(const RawBuffer& key, RdbDataIterator& iterator) {
				m_db.find(key, iterator);
				return true;
			}

			bool remove(const RawBuffer& key) {
				m_db.RemoveParams.push(key);
				return true;
			}

		private:
			MockDb& m_db;
		};

		auto CreateContainer(MockDb& db) {
			return RdbTypedColumnContainer<ColumnDescriptor, MockContainer>(db, 0);
		}
	}

	// region adapter tests

	TEST(TEST_CLASS, SizeForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);
		db.Size = 12345u;

		// Act:
		auto size = container.size();
		auto isEmpty = container.empty();

		// Assert:
		EXPECT_EQ(12345u, size);
		EXPECT_FALSE(isEmpty);
	}

	TEST(TEST_CLASS, EmptyForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);
		db.Size = 0u;

		// Act:
		auto size = container.size();
		auto isEmpty = container.empty();

		// Assert:
		EXPECT_EQ(0u, size);
		EXPECT_TRUE(isEmpty);
	}

	TEST(TEST_CLASS, SaveSizeForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		container.saveSize(87654321);

		// Assert:
		EXPECT_EQ(87654321u, db.SavedSize);
	}

	TEST(TEST_CLASS, InsertSerializesKeyAndValueAndForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		auto keyValue = std::make_pair<std::string, DummyValue>("hello", { "hello", 456, 3.1415 });
		container.insert(keyValue);

		// Assert:
		ASSERT_EQ(1u, db.InsertParams.params().size());
		const auto& params = db.InsertParams.params()[0];
		const auto& key = keyValue.first;
		EXPECT_EQ(MutatePointer(key.data()), params.Key.pData);
		EXPECT_EQ(MutateSize(key.size()), params.Key.Size);
		EXPECT_EQ("456 3.14", params.Value);
	}

	TEST(TEST_CLASS, FindSerializesKeyAndForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		std::string key = "hello";
		auto iter = container.find(key);

		// Assert:
		ASSERT_EQ(1u, db.FindParams.params().size());
		const auto& params = db.FindParams.params()[0];
		EXPECT_EQ(MutatePointer(key.data()), params.Key.pData);
		EXPECT_EQ(MutateSize(key.size()), params.Key.Size);
		EXPECT_EQ(&iter.dbIterator(), params.pIterator);
	}

	TEST(TEST_CLASS, RemoveSerializesKeyAndForwardsToContainer) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		std::string key = "hello";
		container.remove(key);

		// Assert:
		ASSERT_EQ(1u, db.RemoveParams.params().size());
		const auto& params = db.RemoveParams.params()[0];
		EXPECT_EQ(MutatePointer(key.data()), params.Key.pData);
		EXPECT_EQ(MutateSize(key.size()), params.Key.Size);
	}

	TEST(TEST_CLASS, CendReturnsUnitializedIterator) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		auto iter = container.cend();

		// Assert: empty iterator is uninitialized by default
		decltype(container)::const_iterator iterator;
		EXPECT_EQ(iterator, iter);
	}

	// endregion

	// region iterator tests

	TEST(TEST_CLASS, ConstAndNonConstDbIteratorReturnSameObject) {
		// Arrange:
		using IteratorType = RdbTypedColumnContainer<ColumnDescriptor, MockContainer>::const_iterator;
		IteratorType iterator;

		// Act:
		const auto& constDbIter = const_cast<const IteratorType&>(iterator).dbIterator();
		auto& nonConstDbIter = iterator.dbIterator();

		// Assert:
		EXPECT_EQ(&constDbIter, &nonConstDbIter);
	}

	TEST(TEST_CLASS, DereferenceOfInvalidIteratorThrows) {
		// Arrange:
		MockDb db;
		auto container = CreateContainer(db);

		// Act:
		auto iter = container.find("hello");

		// Assert:
		EXPECT_EQ(RdbDataIterator::End(), iter.dbIterator());
		EXPECT_EQ(container.cend(), iter);
		EXPECT_THROW(*iter, catapult_invalid_argument);
	}

	TEST(TEST_CLASS, DereferenceForwardsToDeserializer) {
		// Arrange:
		MockDb db(true);
		auto container = CreateContainer(db);

		// Act:
		auto iter = container.find("hello");

		// Assert: dereferenced value contains dummy data set by deserializer
		ASSERT_NE(container.cend(), iter);
		const auto& keyValuePair = *iter;
		EXPECT_EQ("world", keyValuePair.first);
		EXPECT_EQ("world", keyValuePair.second.KeyCopy);
		EXPECT_EQ(54321, keyValuePair.second.Integer);
		EXPECT_EQ(2.718281, keyValuePair.second.FloatingPoint);
	}

	TEST(TEST_CLASS, DereferenceAndArrowReturnSameObject) {
		// Arrange:
		MockDb db(true);
		auto container = CreateContainer(db);

		// Act:
		auto iter = container.find("hello");

		// Assert:
		ASSERT_NE(container.cend(), iter);
		const auto& keyValuePair = *iter;
		const auto* pKeyValuePair = iter.operator->();

		EXPECT_EQ(&keyValuePair, pKeyValuePair);
	}

	// endregion
}}