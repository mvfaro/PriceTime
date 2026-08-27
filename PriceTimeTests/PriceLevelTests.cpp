#include "pch.h"
#include "PriceLevel.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

namespace {

    lob::Order makeOrder(
        lob::OrderId id,
        lob::Quantity quantity,
        lob::Sequence sequence
    ) {
        return lob::Order(
            id,
            lob::Side::Buy,
            1050,
            quantity,
            sequence
        );
    }

    TEST(PriceLevelTests, ConstructorCreatesEmptyLevel) {
        const lob::PriceLevel level(1050);

        EXPECT_EQ(level.price(), 1050);
        EXPECT_EQ(level.totalQuantity(), 0);
        EXPECT_EQ(level.orderCount(), 0);
        EXPECT_TRUE(level.empty());
    }

    TEST(PriceLevelTests, AppendAddsOrderAndUpdatesTotals) {
        lob::PriceLevel level(1050);

        const auto position =
            level.append(makeOrder(1, 100, 1));

        EXPECT_FALSE(level.empty());
        EXPECT_EQ(level.orderCount(), 1);
        EXPECT_EQ(level.totalQuantity(), 100);
        EXPECT_EQ(position->id(), 1);
        EXPECT_EQ(level.front().id(), 1);
    }

    TEST(PriceLevelTests, OrdersMaintainFifoPriority) {
        lob::PriceLevel level(1050);

        const auto first =
            level.append(makeOrder(1, 100, 1));

        level.append(makeOrder(2, 75, 2));

        EXPECT_EQ(level.front().id(), 1);

        level.erase(first);

        EXPECT_EQ(level.front().id(), 2);
        EXPECT_EQ(level.totalQuantity(), 75);
        EXPECT_EQ(level.orderCount(), 1);
    }

    TEST(PriceLevelTests, ReduceUpdatesQuantities) {
        lob::PriceLevel level(1050);

        const auto position =
            level.append(makeOrder(1, 100, 1));

        level.reduce(position, 40);

        EXPECT_EQ(position->remainingQuantity(), 60);
        EXPECT_EQ(level.totalQuantity(), 60);
        EXPECT_FALSE(position->isFilled());
    }

    TEST(PriceLevelTests, ExcessiveReductionThrows) {
        lob::PriceLevel level(1050);

        const auto position =
            level.append(makeOrder(1, 100, 1));

        EXPECT_THROW(
            level.reduce(position, 101),
            std::invalid_argument
        );

        EXPECT_EQ(position->remainingQuantity(), 100);
        EXPECT_EQ(level.totalQuantity(), 100);
    }

    TEST(PriceLevelTests, FilledOrderRequiresExplicitErase) {
        lob::PriceLevel level(1050);

        const auto position =
            level.append(makeOrder(1, 100, 1));

        level.reduce(position, 100);

        EXPECT_TRUE(position->isFilled());
        EXPECT_EQ(level.totalQuantity(), 0);
        EXPECT_EQ(level.orderCount(), 1);

        level.erase(position);

        EXPECT_TRUE(level.empty());
        EXPECT_EQ(level.orderCount(), 0);
    }

    TEST(PriceLevelTests, FrontOfEmptyLevelThrows) {
        lob::PriceLevel level(1050);

        EXPECT_THROW(
            level.front(),
            std::runtime_error
        );
    }

    TEST(PriceLevelTests, ZeroPriceThrows) {
        EXPECT_THROW(
            (lob::PriceLevel{ 0 }),
            std::invalid_argument
        );
    }

} // anonymous namespace