#pragma once
#include "Order.hpp"
#include <cstddef>
#include <list>

namespace lob {

    class PriceLevel {
    public:
        using OrderQueue = std::list<Order>;
        using Iterator = OrderQueue::iterator;
        using ConstIterator = OrderQueue::const_iterator;

        explicit PriceLevel(Price price);

        Iterator append(Order order);
        void reduce(Iterator order, Quantity amount);
        void erase(Iterator order);

        [[nodiscard]] Order& front();
        [[nodiscard]] const Order& front() const;

        [[nodiscard]] Iterator begin() noexcept;
        [[nodiscard]] ConstIterator begin() const noexcept;

        [[nodiscard]] Price price() const noexcept;
        [[nodiscard]] Quantity totalQuantity() const noexcept;
        [[nodiscard]] std::size_t orderCount() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        Price price_;
        Quantity totalQuantity_{ 0 };
        OrderQueue orders_;
    };

} // namespace lob