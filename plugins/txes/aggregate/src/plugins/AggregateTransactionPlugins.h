#pragma once
#include "catapult/plugins.h"
#include <memory>

namespace catapult {
	namespace model {
		class TransactionPlugin;
		class TransactionRegistry;
	}
}

namespace catapult { namespace plugins {

	/// Creates an aggregate transaction plugin around \a transactionRegistry.
	PLUGIN_API
	std::unique_ptr<model::TransactionPlugin> CreateAggregateTransactionPlugin(const model::TransactionRegistry& transactionRegistry);
}}
