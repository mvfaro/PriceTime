#include "OrderBook.hpp"

#include <algorithm>
#include <utility>

namespace lob {

    ExecutionResult OrderBook::submit(
        const NewOrder& request
    ) {
        ExecutionResult result;

        if (request.price <= 0) {
            result.rejectReason = RejectReason::InvalidPrice;
            return result;
        }

        if (request.quantity == 0) {
            result.rejectReason = RejectReason::InvalidQuantity;
            return result;
        }

        if (orderIndex_.contains(request.id)) {
            result.rejectReason =
                RejectReason::DuplicateOrderId;

            return result;
        }

        Order incoming(
            request.id,
            request.side,
            request.price,
            request.quantity,
            nextOrderSequence_++
        );

        if (request.side == Side::Buy) {
            return matchBuy(std::move(incoming));
        }

        return matchSell(std::move(incoming));
    }

    ExecutionResult OrderBook::matchBuy(Order incoming) {
        ExecutionResult result;
        result.accepted = true;

        while (!incoming.isFilled() && !asks_.empty()) {
            auto bestLevelIterator = asks_.begin();

            if (incoming.price() < bestLevelIterator->first) {
                break;
            }

            PriceLevel& level = bestLevelIterator->second;
            auto restingOrder = level.begin();

            const OrderId makerId = restingOrder->id();
            const Price executionPrice = restingOrder->price();

            const Quantity executedQuantity = std::min(
                incoming.remainingQuantity(),
                restingOrder->remainingQuantity()
            );

            incoming.reduce(executedQuantity);
            level.reduce(restingOrder, executedQuantity);

            result.trades.push_back(
                Trade{
                    nextTradeId_++,
                    makerId,
                    incoming.id(),
                    Side::Buy,
                    executionPrice,
                    executedQuantity,
                    nextTradeSequence_++
                }
            );

            if (restingOrder->isFilled()) {
                orderIndex_.erase(makerId);
                level.erase(restingOrder);
            }

            if (level.empty()) {
                asks_.erase(bestLevelIterator);
            }
        }

        if (!incoming.isFilled()) {
            result.restingQuantity =
                incoming.remainingQuantity();

            restOrder(std::move(incoming));
        }

        return result;
    }

    ExecutionResult OrderBook::matchSell(Order incoming) {
        ExecutionResult result;
        result.accepted = true;

        while (!incoming.isFilled() && !bids_.empty()) {
            auto bestLevelIterator = bids_.begin();

            if (incoming.price() > bestLevelIterator->first) {
                break;
            }

            PriceLevel& level = bestLevelIterator->second;
            auto restingOrder = level.begin();

            const OrderId makerId = restingOrder->id();
            const Price executionPrice = restingOrder->price();

            const Quantity executedQuantity = std::min(
                incoming.remainingQuantity(),
                restingOrder->remainingQuantity()
            );

            incoming.reduce(executedQuantity);
            level.reduce(restingOrder, executedQuantity);

            result.trades.push_back(
                Trade{
                    nextTradeId_++,
                    makerId,
                    incoming.id(),
                    Side::Sell,
                    executionPrice,
                    executedQuantity,
                    nextTradeSequence_++
                }
            );

            if (restingOrder->isFilled()) {
                orderIndex_.erase(makerId);
                level.erase(restingOrder);
            }

            if (level.empty()) {
                bids_.erase(bestLevelIterator);
            }
        }

        if (!incoming.isFilled()) {
            result.restingQuantity =
                incoming.remainingQuantity();

            restOrder(std::move(incoming));
        }

        return result;
    }

    void OrderBook::restOrder(Order order) {
        if (order.side() == Side::Buy) {
            auto levelResult = bids_.try_emplace(
                order.price(),
                order.price()
            );

            PriceLevel& level = levelResult.first->second;

            auto position = level.append(
                std::move(order)
            );

            orderIndex_.emplace(
                position->id(),
                OrderLocator{
                    Side::Buy,
                    position->price(),
                    &level,
                    position
                }
            );

            return;
        }

        auto levelResult = asks_.try_emplace(
            order.price(),
            order.price()
        );

        PriceLevel& level = levelResult.first->second;

        auto position = level.append(
            std::move(order)
        );

        orderIndex_.emplace(
            position->id(),
            OrderLocator{
                Side::Sell,
                position->price(),
                &level,
                position
            }
        );
    }

    CancelResult OrderBook::cancel(OrderId orderId) {
        auto foundOrder = orderIndex_.find(orderId);

        if (foundOrder == orderIndex_.end()) {
            return {
                false,
                RejectReason::UnknownOrderId
            };
        }

        OrderLocator locator = foundOrder->second;

        locator.level->erase(locator.position);
        orderIndex_.erase(foundOrder);

        if (locator.level->empty()) {
            if (locator.side == Side::Buy) {
                bids_.erase(locator.price);
            }
            else {
                asks_.erase(locator.price);
            }
        }

        return {
            true,
            RejectReason::None
        };
    }

    ExecutionResult OrderBook::modify(
        const ModifyOrder& request
    ) {
        ExecutionResult result;

        auto foundOrder = orderIndex_.find(request.id);

        if (foundOrder == orderIndex_.end()) {
            result.rejectReason =
                RejectReason::UnknownOrderId;

            return result;
        }

        if (request.newPrice <= 0) {
            result.rejectReason =
                RejectReason::InvalidPrice;

            return result;
        }

        if (request.newQuantity == 0) {
            result.rejectReason =
                RejectReason::InvalidQuantity;

            return result;
        }

        const Side originalSide = foundOrder->second.side;

        const CancelResult cancellation =
            cancel(request.id);

        if (!cancellation.cancelled) {
            result.rejectReason =
                cancellation.rejectReason;

            return result;
        }

        return submit(
            NewOrder{
                request.id,
                originalSide,
                request.newPrice,
                request.newQuantity
            }
        );
    }

    std::optional<Price> OrderBook::bestBid() const {
        if (bids_.empty()) {
            return std::nullopt;
        }

        return bids_.begin()->first;
    }

    std::optional<Price> OrderBook::bestAsk() const {
        if (asks_.empty()) {
            return std::nullopt;
        }

        return asks_.begin()->first;
    }

    std::optional<Price> OrderBook::spread() const {
        if (bids_.empty() || asks_.empty()) {
            return std::nullopt;
        }

        return asks_.begin()->first - bids_.begin()->first;
    }

    std::vector<LevelView> OrderBook::bidDepth(
        std::size_t numberOfLevels
    ) const {
        std::vector<LevelView> depth;

        for (const auto& [price, level] : bids_) {
            if (depth.size() >= numberOfLevels) {
                break;
            }

            depth.push_back(
                LevelView{
                    price,
                    level.totalQuantity(),
                    level.orderCount()
                }
            );
        }

        return depth;
    }

    std::vector<LevelView> OrderBook::askDepth(
        std::size_t numberOfLevels
    ) const {
        std::vector<LevelView> depth;

        for (const auto& [price, level] : asks_) {
            if (depth.size() >= numberOfLevels) {
                break;
            }

            depth.push_back(
                LevelView{
                    price,
                    level.totalQuantity(),
                    level.orderCount()
                }
            );
        }

        return depth;
    }

    bool OrderBook::contains(OrderId orderId) const {
        return orderIndex_.contains(orderId);
    }

    std::size_t OrderBook::orderCount() const noexcept {
        return orderIndex_.size();
    }

    bool OrderBook::validateInvariants() const {
        if (!bids_.empty() && !asks_.empty()) {
            if (bids_.begin()->first >= asks_.begin()->first) {
                return false;
            }
        }

        std::size_t countedOrders = 0;

        for (const auto& [price, level] : bids_) {
            if (
                level.empty() ||
                level.price() != price ||
                level.totalQuantity() == 0
                ) {
                return false;
            }

            countedOrders += level.orderCount();
        }

        for (const auto& [price, level] : asks_) {
            if (
                level.empty() ||
                level.price() != price ||
                level.totalQuantity() == 0
                ) {
                return false;
            }

            countedOrders += level.orderCount();
        }

        return countedOrders == orderIndex_.size();
    }

} // namespace lob