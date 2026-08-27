#include "pch.h"
#include "MatchingEngine.hpp"

#include <gtest/gtest.h>

namespace {

    lob::NewOrder makeOrder(
        lob::OrderId id,
        lob::Side side,
        lob::Price price,
        lob::Quantity quantity
    ) {
        return {
            id,
            side,
            price,
            quantity
        };
    }

    TEST(MatchingEngineTests, StartsWithoutBooks) {
        const lob::MatchingEngine engine;

        EXPECT_EQ(engine.bookCount(), 0);
        EXPECT_EQ(engine.findBook("AAPL"), nullptr);
    }

    TEST(MatchingEngineTests, SubmitCreatesBook) {
        lob::MatchingEngine engine;

        const auto result = engine.submit(
            "AAPL",
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        EXPECT_TRUE(result.accepted);
        EXPECT_EQ(engine.bookCount(), 1);

        const lob::OrderBook* book =
            engine.findBook("AAPL");

        ASSERT_NE(book, nullptr);
        EXPECT_TRUE(book->contains(1));
        EXPECT_EQ(book->orderCount(), 1);
    }

    TEST(MatchingEngineTests, SymbolsCanReuseOrderIds) {
        lob::MatchingEngine engine;

        const auto appleResult = engine.submit(
            "AAPL",
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        const auto microsoftResult = engine.submit(
            "MSFT",
            makeOrder(1, lob::Side::Sell, 1100, 50)
        );

        EXPECT_TRUE(appleResult.accepted);
        EXPECT_TRUE(microsoftResult.accepted);
        EXPECT_EQ(engine.bookCount(), 2);

        const lob::OrderBook* appleBook =
            engine.findBook("AAPL");

        const lob::OrderBook* microsoftBook =
            engine.findBook("MSFT");

        ASSERT_NE(appleBook, nullptr);
        ASSERT_NE(microsoftBook, nullptr);

        EXPECT_TRUE(appleBook->contains(1));
        EXPECT_TRUE(microsoftBook->contains(1));
    }

    TEST(MatchingEngineTests, OrdersDoNotMatchAcrossSymbols) {
        lob::MatchingEngine engine;

        engine.submit(
            "AAPL",
            makeOrder(1, lob::Side::Sell, 1000, 50)
        );

        const auto result = engine.submit(
            "MSFT",
            makeOrder(2, lob::Side::Buy, 1100, 50)
        );

        EXPECT_TRUE(result.accepted);
        EXPECT_TRUE(result.trades.empty());
        EXPECT_EQ(result.restingQuantity, 50);

        const lob::OrderBook* appleBook =
            engine.findBook("AAPL");

        const lob::OrderBook* microsoftBook =
            engine.findBook("MSFT");

        ASSERT_NE(appleBook, nullptr);
        ASSERT_NE(microsoftBook, nullptr);

        EXPECT_TRUE(appleBook->contains(1));
        EXPECT_TRUE(microsoftBook->contains(2));
    }

    TEST(MatchingEngineTests, CancelOnlyAffectsRequestedSymbol) {
        lob::MatchingEngine engine;

        engine.submit(
            "AAPL",
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        engine.submit(
            "MSFT",
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        const auto result = engine.cancel("AAPL", 1);

        EXPECT_TRUE(result.cancelled);

        const lob::OrderBook* appleBook =
            engine.findBook("AAPL");

        const lob::OrderBook* microsoftBook =
            engine.findBook("MSFT");

        ASSERT_NE(appleBook, nullptr);
        ASSERT_NE(microsoftBook, nullptr);

        EXPECT_FALSE(appleBook->contains(1));
        EXPECT_TRUE(microsoftBook->contains(1));
    }

    TEST(MatchingEngineTests, ModifyOnlyAffectsRequestedSymbol) {
        lob::MatchingEngine engine;

        engine.submit(
            "AAPL",
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        engine.submit(
            "MSFT",
            makeOrder(1, lob::Side::Buy, 900, 100)
        );

        const auto result = engine.modify(
            "AAPL",
            lob::ModifyOrder{
                1,
                1050,
                40
            }
        );

        EXPECT_TRUE(result.accepted);
        EXPECT_EQ(result.restingQuantity, 40);

        const lob::OrderBook* appleBook =
            engine.findBook("AAPL");

        const lob::OrderBook* microsoftBook =
            engine.findBook("MSFT");

        ASSERT_NE(appleBook, nullptr);
        ASSERT_NE(microsoftBook, nullptr);

        ASSERT_TRUE(appleBook->bestBid().has_value());
        ASSERT_TRUE(microsoftBook->bestBid().has_value());

        EXPECT_EQ(*appleBook->bestBid(), 1050);
        EXPECT_EQ(*microsoftBook->bestBid(), 900);
    }

    TEST(
        MatchingEngineTests,
        UnknownSymbolOperationsDoNotCreateBook
    ) {
        lob::MatchingEngine engine;

        const auto cancellation =
            engine.cancel("UNKNOWN", 1);

        const auto modification = engine.modify(
            "UNKNOWN",
            lob::ModifyOrder{
                1,
                1000,
                100
            }
        );

        EXPECT_FALSE(cancellation.cancelled);
        EXPECT_EQ(
            cancellation.rejectReason,
            lob::RejectReason::UnknownOrderId
        );

        EXPECT_FALSE(modification.accepted);
        EXPECT_EQ(
            modification.rejectReason,
            lob::RejectReason::UnknownOrderId
        );

        EXPECT_EQ(engine.bookCount(), 0);
        EXPECT_EQ(engine.findBook("UNKNOWN"), nullptr);
    }

} // anonymous namespace