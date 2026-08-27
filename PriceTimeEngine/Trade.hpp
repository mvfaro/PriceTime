#pragma once

#include "Types.hpp"

#include <cstddef>
#include <vector>

namespace lob {

    struct Trade {
        TradeId tradeId;
        OrderId makerOrderId;
        OrderId takerOrderId;
        Side aggressorSide;
        Price price;
        Quantity quantity;
        Sequence sequence;
    };

    struct ExecutionResult {
        bool accepted{ false };
        RejectReason rejectReason{ RejectReason::None };
        Quantity restingQuantity{ 0 };
        std::vector<Trade> trades;
    };

    struct CancelResult {
        bool cancelled{ false };
        RejectReason rejectReason{ RejectReason::None };
    };

    struct LevelView {
        Price price;
        Quantity totalQuantity;
        std::size_t orderCount;
    };

} // namespace lob
