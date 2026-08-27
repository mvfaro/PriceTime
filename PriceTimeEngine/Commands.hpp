#pragma once

#include "Types.hpp"

#include <cstddef>
#include <string>
#include <variant>

namespace lob {

    struct NewOrder {
        OrderId id;
        Side side;
        Price price;
        Quantity quantity;
    };

    struct CancelOrder {
        OrderId id;
    };

    struct ModifyOrder {
        OrderId id;
        Price newPrice;
        Quantity newQuantity;
    };

    struct SubmitCommand {
        std::string symbol;
        NewOrder order;
    };

    struct CancelCommand {
        std::string symbol;
        CancelOrder cancellation;
    };

    struct ModifyCommand {
        std::string symbol;
        ModifyOrder modification;
    };

    struct BookCommand {
        std::string symbol;
        std::size_t levels;
    };

    struct QuitCommand {
    };

    using Command = std::variant<
        SubmitCommand,
        CancelCommand,
        ModifyCommand,
        BookCommand,
        QuitCommand
    >;

} // namespace lob