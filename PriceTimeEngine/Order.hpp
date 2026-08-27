#pragma once

#include "Types.hpp"

namespace lob {

    class Order {
    public:
        Order(
            OrderId id,
            Side side,
            Price price,
            Quantity quantity,
            Sequence sequence
        );

        [[nodiscard]] OrderId id() const noexcept;
        [[nodiscard]] Side side() const noexcept;
        [[nodiscard]] Price price() const noexcept;
        [[nodiscard]] Quantity originalQuantity() const noexcept;
        [[nodiscard]] Quantity remainingQuantity() const noexcept;
        [[nodiscard]] Sequence sequence() const noexcept;
        [[nodiscard]] bool isFilled() const noexcept;

        void reduce(Quantity amount);

    private:
        OrderId id_;
        Side side_;
        Price price_;
        Quantity originalQuantity_;
        Quantity remainingQuantity_;
        Sequence sequence_;
    };

} // namespace lob