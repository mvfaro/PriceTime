#pragma once

#include "Commands.hpp"
#include "PriceLevel.hpp"
#include "Trade.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace lob {

    class OrderBook {
    public:
        OrderBook() = default;

        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;
        OrderBook(OrderBook&&) = delete;
        OrderBook& operator=(OrderBook&&) = delete;

        ExecutionResult submit(const NewOrder& request);
        CancelResult cancel(OrderId orderId);
        ExecutionResult modify(const ModifyOrder& request);

        [[nodiscard]] std::optional<Price> bestBid() const;
        [[nodiscard]] std::optional<Price> bestAsk() const;
        [[nodiscard]] std::optional<Price> spread() const;

        [[nodiscard]] std::vector<LevelView>
            bidDepth(std::size_t numberOfLevels) const;

        [[nodiscard]] std::vector<LevelView>
            askDepth(std::size_t numberOfLevels) const;

        [[nodiscard]] bool contains(OrderId orderId) const;
        [[nodiscard]] std::size_t orderCount() const noexcept;
        [[nodiscard]] bool validateInvariants() const;

    private:
        using BidLevels =
            std::map<Price, PriceLevel, std::greater<Price>>;

        using AskLevels =
            std::map<Price, PriceLevel, std::less<Price>>;

        struct OrderLocator {
            Side side;
            Price price;
            PriceLevel* level;
            PriceLevel::Iterator position;
        };

        ExecutionResult matchBuy(Order incoming);
        ExecutionResult matchSell(Order incoming);

        void restOrder(Order order);

        BidLevels bids_;
        AskLevels asks_;

        std::unordered_map<OrderId, OrderLocator> orderIndex_;

        Sequence nextOrderSequence_{ 1 };
        Sequence nextTradeSequence_{ 1 };
        TradeId nextTradeId_{ 1 };
    };

} // namespace lob#pragma once
