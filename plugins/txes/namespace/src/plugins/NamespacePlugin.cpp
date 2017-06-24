#include "NamespacePlugin.h"
#include "MosaicDefinitionTransactionPlugins.h"
#include "MosaicSupplyChangeTransactionPlugins.h"
#include "RegisterNamespaceTransactionPlugins.h"
#include "src/cache/MosaicCacheStorage.h"
#include "src/cache/NamespaceCacheStorage.h"
#include "src/config/NamespaceConfiguration.h"
#include "src/handlers/NamespaceDiagnosticHandlers.h"
#include "src/model/NamespaceLifetimeConstraints.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/model/Address.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	namespace {
		constexpr auto GetPluginName() {
			return "catapult.plugins.namespace";
		}

		ArtifactDuration ToDuration(const utils::BlockSpan& blockSpan, const utils::TimeSpan& generationTargetTime) {
			return ArtifactDuration(blockSpan.blocks32(generationTargetTime));
		}

		// region mosaic

		plugins::MosaicRentalFeeConfiguration ToMosaicRentalFeeConfiguration(
				const model::NetworkInfo& network,
				const config::NamespaceConfiguration& config) {
			plugins::MosaicRentalFeeConfiguration rentalFeeConfig;
			rentalFeeConfig.SinkPublicKey = config.MosaicRentalFeeSinkPublicKey;
			rentalFeeConfig.SinkAddress = PublicKeyToAddress(rentalFeeConfig.SinkPublicKey, network.Identifier);
			rentalFeeConfig.Fee = config.MosaicRentalFee;
			rentalFeeConfig.NemesisPublicKey = network.PublicKey;
			return rentalFeeConfig;
		}

		auto GetMosaicView(const cache::CatapultCache& cache) {
			return cache.sub<cache::MosaicCache>().createView();
		}

		void RegisterMosaicSubsystem(PluginManager& manager, const config::NamespaceConfiguration& config) {
			auto rentalFeeConfig = ToMosaicRentalFeeConfiguration(manager.config().Network, config);
			manager.addTransactionSupport(CreateMosaicDefinitionTransactionPlugin(rentalFeeConfig));
			manager.addTransactionSupport(CreateMosaicSupplyChangeTransactionPlugin());

			manager.addCacheSupport<cache::MosaicCacheStorage>(std::make_unique<cache::MosaicCache>());

			manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("MOSAIC C"), [&cache]() { return GetMosaicView(cache)->size(); });
				counters.emplace_back(utils::DiagnosticCounterId("MOSAIC C DS"), [&cache]() { return GetMosaicView(cache)->deepSize(); });
			});

			auto maxDuration = ToDuration(config.MaxMosaicDuration, manager.config().BlockGenerationTargetTime);
			manager.addStatelessValidatorHook([config, maxDuration](auto& builder) {
				builder
					.add(validators::CreateMosaicNameValidator(config.MaxNameSize))
					.add(validators::CreateMosaicPropertiesValidator(config.MaxMosaicDivisibility, maxDuration))
					.add(validators::CreateMosaicSupplyChangeValidator());
			});

			manager.addStatefulValidatorHook([maxDivisibleUnits = config.MaxMosaicDivisibleUnits](auto& builder) {
				builder
					.add(validators::CreateMosaicChangeAllowedValidator())
					.add(validators::CreateNamespaceMosaicConsistencyValidator())
					.add(validators::CreateMosaicAvailabilityValidator())
					.add(validators::CreateMosaicTransferValidator())
					// note that the following validator depends on MosaicChangeAllowedValidator
					.add(validators::CreateMosaicSupplyChangeAllowedValidator(maxDivisibleUnits));
			});

			auto maxRollbackBlocks = manager.config().MaxRollbackBlocks;
			manager.addObserverHook([maxRollbackBlocks](auto& builder) {
				builder
					.add(observers::CreateMosaicDefinitionObserver())
					.add(observers::CreateMosaicSupplyChangeObserver())
					.add(observers::CreateMosaicPruningObserver(maxRollbackBlocks))
					.add(observers::CreateNemesisBalanceChangeObserver());
			});
		}

		// endregion

		// region namespace

		plugins::NamespaceRentalFeeConfiguration ToNamespaceRentalFeeConfiguration(
				const model::NetworkInfo& network,
				const config::NamespaceConfiguration& config) {
			plugins::NamespaceRentalFeeConfiguration rentalFeeConfig;
			rentalFeeConfig.SinkPublicKey = config.NamespaceRentalFeeSinkPublicKey;
			rentalFeeConfig.SinkAddress = PublicKeyToAddress(rentalFeeConfig.SinkPublicKey, network.Identifier);
			rentalFeeConfig.RootFeePerBlock = config.RootNamespaceRentalFeePerBlock;
			rentalFeeConfig.ChildFee = config.ChildNamespaceRentalFee;
			rentalFeeConfig.NemesisPublicKey = network.PublicKey;
			return rentalFeeConfig;
		}

		auto GetNamespaceView(const cache::CatapultCache& cache) {
			return cache.sub<cache::NamespaceCache>().createView();
		}

		void RegisterNamespaceSubsystem(PluginManager& manager, const config::NamespaceConfiguration& config) {
			auto rentalFeeConfig = ToNamespaceRentalFeeConfiguration(manager.config().Network, config);
			manager.addTransactionSupport(CreateRegisterNamespaceTransactionPlugin(rentalFeeConfig));

			manager.addCacheSupport<cache::NamespaceCacheStorage>(std::make_unique<cache::NamespaceCache>());

			manager.addDiagnosticHandlerHook([](auto& handlers, const cache::CatapultCache& cache) {
				handlers::RegisterNamespaceInfosHandler(
						handlers,
						handlers::CreateNamespaceInfosSupplier(cache.sub<cache::NamespaceCache>()));
				handlers::RegisterMosaicInfosHandler(
						handlers,
						handlers::CreateMosaicInfosSupplier(cache.sub<cache::MosaicCache>()));
			});

			manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
				counters.emplace_back(utils::DiagnosticCounterId("NS C"), [&cache]() { return GetNamespaceView(cache)->size(); });
				counters.emplace_back(utils::DiagnosticCounterId("NS C AS"), [&cache]() { return GetNamespaceView(cache)->activeSize(); });
				counters.emplace_back(utils::DiagnosticCounterId("NS C DS"), [&cache]() { return GetNamespaceView(cache)->deepSize(); });
			});

			auto maxDuration = ToDuration(config.MaxNamespaceDuration, manager.config().BlockGenerationTargetTime);
			manager.addStatelessValidatorHook([config, maxDuration](auto& builder) {
				const auto& reservedNames = config.ReservedRootNamespaceNames;
				builder
					.add(validators::CreateNamespaceTypeValidator())
					.add(validators::CreateNamespaceNameValidator(config.MaxNameSize))
					.add(validators::CreateRootNamespaceValidator(maxDuration, reservedNames));
			});

			auto gracePeriodDuration = ToDuration(config.NamespaceGracePeriodDuration, manager.config().BlockGenerationTargetTime);
			model::NamespaceLifetimeConstraints constraints(maxDuration, gracePeriodDuration, manager.config().MaxRollbackBlocks);
			manager.addStatefulValidatorHook([constraints](auto& builder) {
				builder
					.add(validators::CreateRootNamespaceAvailabilityValidator(constraints))
					.add(validators::CreateChildNamespaceAvailabilityValidator());
			});

			auto pruneInterval = manager.config().BlockPruneInterval;
			manager.addObserverHook([constraints, pruneInterval](auto& builder) {
				builder
					.add(observers::CreateRegisterNamespaceMosaicPruningObserver(constraints))
					.add(observers::CreateRootNamespaceObserver())
					.add(observers::CreateChildNamespaceObserver())
					.add(observers::CreateNamespacePruningObserver(constraints.TotalGracePeriodDuration, pruneInterval));
			});
		}

		// endregion
	}

	void RegisterNamespaceSubsystem(PluginManager& manager) {
		auto config = model::LoadPluginConfiguration<config::NamespaceConfiguration>(manager.config(), GetPluginName());
		RegisterNamespaceSubsystem(manager, config);
		RegisterMosaicSubsystem(manager, config);
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterNamespaceSubsystem(manager);
}
