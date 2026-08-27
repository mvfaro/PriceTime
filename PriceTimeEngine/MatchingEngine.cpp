#include "MatchingEngine.hpp"

#include <memory>

namespace lob {

    OrderBook& MatchingEngine::getOrCreateBook(
        const std::string& symbol
    ) {
        auto [position, inserted] =
            books_.try_emplace(symbol);

        if (inserted) {
            position->second =
                std::make_unique<OrderBook>();
        }

        return *position->second;
    }

    ExecutionResult MatchingEngine::submit(
        const std::string& symbol,
        const NewOrder& order
    ) {
        OrderBook& book = getOrCreateBook(symbol);
        return book.submit(order);
    }

    CancelResult MatchingEngine::cancel(
        const std::string& symbol,
        OrderId orderId
    ) {
        auto foundBook = books_.find(symbol);

        if (foundBook == books_.end()) {
            return {
                false,
                RejectReason::UnknownOrderId
            };
        }

        return foundBook->second->cancel(orderId);
    }

    ExecutionResult MatchingEngine::modify(
        const std::string& symbol,
        const ModifyOrder& order
    ) {
        auto foundBook = books_.find(symbol);

        if (foundBook == books_.end()) {
            ExecutionResult result;
            result.rejectReason =
                RejectReason::UnknownOrderId;

            return result;
        }

        return foundBook->second->modify(order);
    }

    OrderBook* MatchingEngine::findBook(
        const std::string& symbol
    ) {
        auto foundBook = books_.find(symbol);

        if (foundBook == books_.end()) {
            return nullptr;
        }

        return foundBook->second.get();
    }

    const OrderBook* MatchingEngine::findBook(
        const std::string& symbol
    ) const {
        auto foundBook = books_.find(symbol);

        if (foundBook == books_.end()) {
            return nullptr;
        }

        return foundBook->second.get();
    }

    std::size_t MatchingEngine::bookCount() const noexcept {
        return books_.size();
    }

} // namespace lob