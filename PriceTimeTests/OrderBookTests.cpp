#include "pch.h"
#include "OrderBook.hpp"

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

    TEST(OrderBookTests, EmptyBookStartsValid) {
        const lob::OrderBook book;

        EXPECT_FALSE(book.bestBid().has_value());
        EXPECT_FALSE(book.bestAsk().has_value());
        EXPECT_FALSE(book.spread().has_value());
        EXPECT_EQ(book.orderCount(), 0);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, NonCrossingOrdersCreateSpread) {
        lob::OrderBook book;

        const auto buyResult = book.submit(
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        const auto sellResult = book.submit(
            makeOrder(2, lob::Side::Sell, 1020, 50)
        );

        EXPECT_TRUE(buyResult.accepted);
        EXPECT_TRUE(sellResult.accepted);
        EXPECT_EQ(buyResult.restingQuantity, 100);
        EXPECT_EQ(sellResult.restingQuantity, 50);

        ASSERT_TRUE(book.bestBid().has_value());
        ASSERT_TRUE(book.bestAsk().has_value());
        ASSERT_TRUE(book.spread().has_value());

        EXPECT_EQ(*book.bestBid(), 1000);
        EXPECT_EQ(*book.bestAsk(), 1020);
        EXPECT_EQ(*book.spread(), 20);
        EXPECT_EQ(book.orderCount(), 2);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, IncomingBuyMatchesAtMakerPrice) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Sell, 1050, 100)
        );

        const auto result = book.submit(
            makeOrder(2, lob::Side::Buy, 1060, 40)
        );

        ASSERT_TRUE(result.accepted);
        ASSERT_EQ(result.trades.size(), 1);

        const lob::Trade& trade = result.trades.front();

        EXPECT_EQ(trade.makerOrderId, 1);
        EXPECT_EQ(trade.takerOrderId, 2);
        EXPECT_EQ(trade.aggressorSide, lob::Side::Buy);
        EXPECT_EQ(trade.price, 1050);
        EXPECT_EQ(trade.quantity, 40);
        EXPECT_EQ(result.restingQuantity, 0);

        EXPECT_TRUE(book.contains(1));
        EXPECT_FALSE(book.contains(2));

        const auto asks = book.askDepth(1);

        ASSERT_EQ(asks.size(), 1);
        EXPECT_EQ(asks.front().totalQuantity, 60);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, IncomingSellMatchesAtMakerPrice) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Buy, 1050, 70)
        );

        const auto result = book.submit(
            makeOrder(2, lob::Side::Sell, 1000, 70)
        );

        ASSERT_TRUE(result.accepted);
        ASSERT_EQ(result.trades.size(), 1);

        EXPECT_EQ(result.trades.front().makerOrderId, 1);
        EXPECT_EQ(result.trades.front().takerOrderId, 2);
        EXPECT_EQ(
            result.trades.front().aggressorSide,
            lob::Side::Sell
        );
        EXPECT_EQ(result.trades.front().price, 1050);
        EXPECT_EQ(result.trades.front().quantity, 70);

        EXPECT_EQ(book.orderCount(), 0);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, SamePriceOrdersMatchInFifoOrder) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Sell, 1050, 50)
        );

        book.submit(
            makeOrder(2, lob::Side::Sell, 1050, 50)
        );

        const auto result = book.submit(
            makeOrder(3, lob::Side::Buy, 1050, 75)
        );

        ASSERT_EQ(result.trades.size(), 2);

        EXPECT_EQ(result.trades[0].makerOrderId, 1);
        EXPECT_EQ(result.trades[0].quantity, 50);

        EXPECT_EQ(result.trades[1].makerOrderId, 2);
        EXPECT_EQ(result.trades[1].quantity, 25);

        EXPECT_FALSE(book.contains(1));
        EXPECT_TRUE(book.contains(2));

        const auto asks = book.askDepth(1);

        ASSERT_EQ(asks.size(), 1);
        EXPECT_EQ(asks.front().totalQuantity, 25);
        EXPECT_EQ(asks.front().orderCount, 1);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, BuySweepsLowestAskFirst) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Sell, 1060, 30)
        );

        book.submit(
            makeOrder(2, lob::Side::Sell, 1050, 20)
        );

        const auto result = book.submit(
            makeOrder(3, lob::Side::Buy, 1060, 40)
        );

        ASSERT_EQ(result.trades.size(), 2);

        EXPECT_EQ(result.trades[0].makerOrderId, 2);
        EXPECT_EQ(result.trades[0].price, 1050);
        EXPECT_EQ(result.trades[0].quantity, 20);

        EXPECT_EQ(result.trades[1].makerOrderId, 1);
        EXPECT_EQ(result.trades[1].price, 1060);
        EXPECT_EQ(result.trades[1].quantity, 20);

        const auto asks = book.askDepth(5);

        ASSERT_EQ(asks.size(), 1);
        EXPECT_EQ(asks.front().price, 1060);
        EXPECT_EQ(asks.front().totalQuantity, 10);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, AggressorRemainderRestsInBook) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Sell, 1050, 30)
        );

        const auto result = book.submit(
            makeOrder(2, lob::Side::Buy, 1060, 50)
        );

        ASSERT_EQ(result.trades.size(), 1);

        EXPECT_EQ(result.trades.front().quantity, 30);
        EXPECT_EQ(result.restingQuantity, 20);
        EXPECT_TRUE(book.contains(2));

        ASSERT_TRUE(book.bestBid().has_value());
        EXPECT_EQ(*book.bestBid(), 1060);
        EXPECT_FALSE(book.bestAsk().has_value());
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, CancelRemovesRestingOrder) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        const auto result = book.cancel(1);

        EXPECT_TRUE(result.cancelled);
        EXPECT_EQ(
            result.rejectReason,
            lob::RejectReason::None
        );
        EXPECT_FALSE(book.contains(1));
        EXPECT_EQ(book.orderCount(), 0);
        EXPECT_FALSE(book.bestBid().has_value());
        EXPECT_TRUE(book.validateInvariants());

        const auto repeatedCancellation = book.cancel(1);

        EXPECT_FALSE(repeatedCancellation.cancelled);
        EXPECT_EQ(
            repeatedCancellation.rejectReason,
            lob::RejectReason::UnknownOrderId
        );
    }

    TEST(OrderBookTests, DuplicateOrderIdIsRejected) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Buy, 1000, 100)
        );

        const auto result = book.submit(
            makeOrder(1, lob::Side::Sell, 1000, 50)
        );

        EXPECT_FALSE(result.accepted);
        EXPECT_EQ(
            result.rejectReason,
            lob::RejectReason::DuplicateOrderId
        );
        EXPECT_TRUE(result.trades.empty());
        EXPECT_EQ(book.orderCount(), 1);
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, ModifyLosesPreviousTimePriority) {
        lob::OrderBook book;

        book.submit(
            makeOrder(1, lob::Side::Buy, 1000, 50)
        );

        book.submit(
            makeOrder(2, lob::Side::Buy, 1000, 50)
        );

        const auto modification = book.modify(
            lob::ModifyOrder{
                1,
                1000,
                50
            }
        );

        ASSERT_TRUE(modification.accepted);

        const auto result = book.submit(
            makeOrder(3, lob::Side::Sell, 1000, 50)
        );

        ASSERT_EQ(result.trades.size(), 1);

        EXPECT_EQ(result.trades.front().makerOrderId, 2);
        EXPECT_FALSE(book.contains(2));
        EXPECT_TRUE(book.contains(1));
        EXPECT_TRUE(book.validateInvariants());
    }

    TEST(OrderBookTests, InvalidOrdersAreRejected) {
        lob::OrderBook book;

        const auto invalidPrice = book.submit(
            makeOrder(1, lob::Side::Buy, 0, 100)
        );

        const auto invalidQuantity = book.submit(
            makeOrder(2, lob::Side::Buy, 1000, 0)
        );

        EXPECT_FALSE(invalidPrice.accepted);
        EXPECT_EQ(
            invalidPrice.rejectReason,
            lob::RejectReason::InvalidPrice
        );

        EXPECT_FALSE(invalidQuantity.accepted);
        EXPECT_EQ(
            invalidQuantity.rejectReason,
            lob::RejectReason::InvalidQuantity
        );

        EXPECT_EQ(book.orderCount(), 0);
        EXPECT_TRUE(book.validateInvariants());
    }

} // anonymous namespace