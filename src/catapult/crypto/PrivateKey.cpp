#include "PrivateKey.h"
#include "SecureZero.h"
#include "catapult/utils/HexParser.h"
#include <algorithm>

namespace catapult { namespace crypto {

	namespace {
		class SecureZeroGuard {
		public:
			SecureZeroGuard(Key& key) : SecureZeroGuard(key.data(), key.size())
			{}

			SecureZeroGuard(uint8_t* pData, size_t dataSize) : m_pData(pData), m_dataSize(dataSize)
			{}

			~SecureZeroGuard() {
				SecureZero(m_pData, m_dataSize);
			}

		private:
			uint8_t* m_pData;
			size_t m_dataSize;
		};
	}

	PrivateKey::PrivateKey(PrivateKey&& rhs) {
		SecureZeroGuard guard(rhs.m_key);
		m_key = std::move(rhs.m_key);
	}

	PrivateKey& PrivateKey::operator=(PrivateKey&& rhs) {
		SecureZeroGuard guard(rhs.m_key);
		m_key = std::move(rhs.m_key);
		return *this;
	}

	PrivateKey::~PrivateKey() {
		SecureZero(m_key);
	}

	PrivateKey PrivateKey::FromString(const char* const pRawKey, size_t keySize) {
		PrivateKey key;
		utils::ParseHexStringIntoContainer(pRawKey, keySize, key.m_key);
		return key;
	}

	PrivateKey PrivateKey::FromString(const std::string& str) {
		return FromString(str.c_str(), str.size());
	}

	PrivateKey PrivateKey::FromStringSecure(char* pRawKey, size_t keySize) {
		SecureZeroGuard guard(reinterpret_cast<uint8_t*>(pRawKey), keySize);
		return FromString(pRawKey, keySize);
	}

	PrivateKey PrivateKey::Generate(const std::function<uint8_t()>& generator) {
		PrivateKey key;
		std::generate_n(key.m_key.begin(), key.m_key.size(), generator);
		return key;
	}

	bool PrivateKey::operator==(const PrivateKey& rhs) const {
		return m_key == rhs.m_key;
	}

	bool PrivateKey::operator!=(const PrivateKey& rhs) const {
		return !(*this == rhs);
	}
}}
