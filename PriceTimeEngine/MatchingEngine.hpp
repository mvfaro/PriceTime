#pragma once

#include "OrderBook.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace lob {

    class MatchingEngine {
    public:
        ExecutionResult submit(
            const std::string& symbol,
            const NewOrder& order
        );

        CancelResult cancel(
            const std::string& symbol,
            OrderId orderId
        );

        ExecutionResult modify(
            const std::string& symbol,
            const ModifyOrder& order
        );

        [[nodiscard]] OrderBook*
            findBook(const std::string& symbol);

        [[nodiscard]] const OrderBook*
            findBook(const std::string& symbol) const;

        [[nodiscard]] std::size_t bookCount() const noexcept;

    private:
        OrderBook& getOrCreateBook(
            const std::string& symbol
        );

        std::unordered_map<
            std::string,
            std::unique_ptr<OrderBook>
        > books_;
    };

} // namespace lob
