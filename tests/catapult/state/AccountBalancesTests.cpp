#include "catapult/state/AccountBalances.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace state {

	namespace {
		constexpr MosaicId Test_Mosaic_Id = MosaicId(12345);
		constexpr MosaicId Test_Mosaic_Id2 = MosaicId(54321);
	}

	TEST(AccountBalancesTests, GetReturnsZeroForUnknownMosaics) {
		// Arrange:
		AccountBalances balances;

		// Act:
		auto amount1 = balances.get(Xem_Id);
		auto amount2 = balances.get(Test_Mosaic_Id);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), amount1);
		EXPECT_EQ(Amount(0), amount2);
	}

	// region credit

	TEST(AccountBalancesTests, CreditDoesNotAddZeroBalance) {
		// Arrange:
		AccountBalances balances;

		// Act:
		balances.credit(Xem_Id, Amount(0));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Xem_Id));
	}

	TEST(AccountBalancesTests, CreditIncreasesAmountStored) {
		// Arrange:
		AccountBalances balances;

		// Act:
		balances.credit(Xem_Id, Amount(12345));

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.get(Xem_Id));
	}

	TEST(AccountBalancesTests, InterleavingCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances;

		// Act:
		balances.credit(Xem_Id, Amount(12345));
		balances.credit(Test_Mosaic_Id, Amount(3456));
		balances.credit(Xem_Id, Amount(54321));

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 54321), balances.get(Xem_Id));
		EXPECT_EQ(Amount(3456), balances.get(Test_Mosaic_Id));
	}

	// endregion

	// region debit

	TEST(AccountBalancesTests, CanDebitZeroFromZeroBalance) {
		// Arrange:
		AccountBalances balances;

		// Act:
		balances.debit(Xem_Id, Amount(0));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Xem_Id));
	}

	TEST(AccountBalancesTests, DebitDecreasesAmountStored) {
		// Arrange:
		AccountBalances balances;
		balances.credit(Xem_Id, Amount(12345));

		// Act:
		balances.debit(Xem_Id, Amount(222));

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345 - 222), balances.get(Xem_Id));
	}

	TEST(AccountBalancesTests, FullDebitRemovesTheMosaicFromCache) {
		// Arrange:
		AccountBalances balances;
		Amount amount = Amount(12345);
		balances.credit(Xem_Id, amount);

		// Act:
		balances.debit(Xem_Id, amount);
		auto xemHeld = balances.get(Xem_Id);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), xemHeld);
	}

	TEST(AccountBalancesTests, InterleavingDebitsYieldCorrectState) {
		// Arrange:
		AccountBalances balances;
		balances.credit(Xem_Id, Amount(12345));
		balances.credit(Test_Mosaic_Id, Amount(3456));

		// Act:
		balances.debit(Xem_Id, Amount(222));
		balances.debit(Test_Mosaic_Id, Amount(1111));
		balances.debit(Xem_Id, Amount(111));

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 - 222 - 111), balances.get(Xem_Id));
		EXPECT_EQ(Amount(3456 - 1111), balances.get(Test_Mosaic_Id));
	}

	TEST(AccountBalancesTests, DebitDoesNotAllowUnderflowOfNonZeroBalance) {
		// Arrange:
		AccountBalances balances;
		balances.credit(Xem_Id, Amount(12345));

		// Act:
		EXPECT_THROW(balances.debit(Xem_Id, Amount(12346)), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.get(Xem_Id));
	}

	TEST(AccountBalancesTests, DebitDoesNotAllowUnderflowOfZeroBalance) {
		// Arrange:
		AccountBalances balances;

		// Act:
		EXPECT_THROW(balances.debit(Xem_Id, Amount(222)), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Xem_Id));
	}

	// endregion

	// region credit + debit

	TEST(AccountBalancesTests, InterleavingDebitsAndCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances;
		balances.credit(Xem_Id, Amount(12345));
		balances.credit(Test_Mosaic_Id, Amount(3456));

		// Act:
		balances.debit(Test_Mosaic_Id, Amount(1111));
		balances.credit(Xem_Id, Amount(1111));
		balances.credit(Test_Mosaic_Id2, Amount(0)); // no op
		balances.debit(Xem_Id, Amount(2345));
		balances.debit(Test_Mosaic_Id2, Amount(0)); // no op
		balances.credit(Test_Mosaic_Id, Amount(5432));

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.get(Xem_Id));
		EXPECT_EQ(Amount(3456 - 1111 + 5432), balances.get(Test_Mosaic_Id));
	}

	TEST(AccountBalancesTests, ChainedInterleavingDebitsAndCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances;
		balances
			.credit(Xem_Id, Amount(12345))
			.credit(Test_Mosaic_Id, Amount(3456));

		// Act:
		balances
			.debit(Test_Mosaic_Id, Amount(1111))
			.credit(Xem_Id, Amount(1111))
			.credit(Test_Mosaic_Id2, Amount(0)) // no op
			.debit(Xem_Id, Amount(2345))
			.debit(Test_Mosaic_Id2, Amount(0)) // no op
			.credit(Test_Mosaic_Id, Amount(5432));

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.get(Xem_Id));
		EXPECT_EQ(Amount(3456 - 1111 + 5432), balances.get(Test_Mosaic_Id));
	}

	// endregion

	// region iteration

	TEST(AccountBalancesTests, CanIterateOverAllBalances) {
		// Arrange:
		AccountBalances balances;
		balances
			.credit(Xem_Id, Amount(12345))
			.credit(Test_Mosaic_Id2, Amount(0))
			.credit(Test_Mosaic_Id, Amount(3456));

		// Act:
		auto numBalances = 0u;
		std::map<MosaicId, Amount> iteratedBalances;
		for (const auto& pair : balances) {
			iteratedBalances.emplace(pair);
			++numBalances;
		}

		// Assert:
		EXPECT_EQ(2u, numBalances);
		EXPECT_EQ(2u, iteratedBalances.size());
		EXPECT_EQ(Amount(12345), iteratedBalances[Xem_Id]);
		EXPECT_EQ(Amount(3456), iteratedBalances[Test_Mosaic_Id]);
	}

	// endregion
}}
