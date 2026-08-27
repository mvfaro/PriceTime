#include "PriceLevel.hpp"

#include <stdexcept>
#include <utility>

namespace lob {

    PriceLevel::PriceLevel(Price price)
        : price_(price) {
        if (price <= 0) {
            throw std::invalid_argument(
                "Price must be greater than zero."
            );
        }
    }

    PriceLevel::Iterator PriceLevel::append(Order order) {
        totalQuantity_ += order.remainingQuantity();

        return orders_.insert(
            orders_.end(),
            std::move(order)
        );
    }

    void PriceLevel::reduce(
        Iterator order,
        Quantity amount
    ) {
        if (order == orders_.end()) {
            throw std::invalid_argument(
                "Invalid order iterator."
            );
        }

        if (amount > order->remainingQuantity()) {
            throw std::invalid_argument(
                "Reduction amount exceeds remaining quantity."
            );
        }

        order->reduce(amount);
        totalQuantity_ -= amount;
    }

    void PriceLevel::erase(Iterator order) {
        if (order == orders_.end()) {
            throw std::invalid_argument(
                "Invalid order iterator."
            );
        }

        totalQuantity_ -= order->remainingQuantity();
        orders_.erase(order);
    }

    Order& PriceLevel::front() {
        if (orders_.empty()) {
            throw std::runtime_error(
                "Price level is empty."
            );
        }

        return orders_.front();
    }

    const Order& PriceLevel::front() const {
        if (orders_.empty()) {
            throw std::runtime_error(
                "Price level is empty."
            );
        }

        return orders_.front();
    }

    PriceLevel::Iterator PriceLevel::begin() noexcept {
        return orders_.begin();
    }

    PriceLevel::ConstIterator
        PriceLevel::begin() const noexcept {
        return orders_.begin();
    }

    Price PriceLevel::price() const noexcept {
        return price_;
    }

    Quantity PriceLevel::totalQuantity() const noexcept {
        return totalQuantity_;
    }

    std::size_t PriceLevel::orderCount() const noexcept {
        return orders_.size();
    }

    bool PriceLevel::empty() const noexcept {
        return orders_.empty();
    }

} // namespace lob