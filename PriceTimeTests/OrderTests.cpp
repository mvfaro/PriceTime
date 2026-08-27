#include "pch.h"
#include "Order.hpp"


#include <gtest/gtest.h>
#include <stdexcept>

namespace {

    TEST(OrderTests, ConstructorStoresOrderDetails) {
        const lob::Order order(
            1,
            lob::Side::Buy,
            1050,
            100,
            7
        );

        EXPECT_EQ(order.id(), 1);
        EXPECT_EQ(order.side(), lob::Side::Buy);
        EXPECT_EQ(order.price(), 1050);
        EXPECT_EQ(order.originalQuantity(), 100);
        EXPECT_EQ(order.remainingQuantity(), 100);
        EXPECT_EQ(order.sequence(), 7);
        EXPECT_FALSE(order.isFilled());
    }

    TEST(OrderTests, ReduceUpdatesRemainingQuantity) {
        lob::Order order(
            1,
            lob::Side::Buy,
            1050,
            100,
            1
        );

        order.reduce(40);

        EXPECT_EQ(order.originalQuantity(), 100);
        EXPECT_EQ(order.remainingQuantity(), 60);
        EXPECT_FALSE(order.isFilled());
    }

    TEST(OrderTests, FullReductionFillsOrder) {
        lob::Order order(
            1,
            lob::Side::Buy,
            1050,
            100,
            1
        );

        order.reduce(100);

        EXPECT_EQ(order.remainingQuantity(), 0);
        EXPECT_TRUE(order.isFilled());
    }

    TEST(OrderTests, ExcessiveReductionThrowsException) {
        lob::Order order(
            1,
            lob::Side::Buy,
            1050,
            100,
            1
        );

        EXPECT_THROW(
            order.reduce(101),
            std::invalid_argument
        );

        EXPECT_EQ(order.remainingQuantity(), 100);
    }

    TEST(OrderTests, ZeroInitialQuantityThrowsException) {
        EXPECT_THROW(
            (
                lob::Order{
                    1,
                    lob::Side::Buy,
                    1050,
                    0,
                    1
                }
                ),
            std::invalid_argument
        );
    }

} // anonymous namespace