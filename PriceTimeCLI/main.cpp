#include "MatchingEngine.hpp"
#include "Parser.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

namespace {

    const char* rejectReasonText(
        lob::RejectReason reason
    ) {
        switch (reason) {
        case lob::RejectReason::None:
            return "None";

        case lob::RejectReason::DuplicateOrderId:
            return "Duplicate order ID";

        case lob::RejectReason::UnknownOrderId:
            return "Unknown order ID";

        case lob::RejectReason::InvalidPrice:
            return "Invalid price";

        case lob::RejectReason::InvalidQuantity:
            return "Invalid quantity";
        }

        return "Unknown error";
    }

    const char* sideText(lob::Side side) {
        return side == lob::Side::Buy
            ? "BUY"
            : "SELL";
    }

    std::string formatPrice(lob::Price price) {
        std::ostringstream output;

        output << price / 100
            << '.'
            << std::setw(2)
            << std::setfill('0')
            << price % 100;

        return output.str();
    }

    void printExecutionResult(
        const lob::ExecutionResult& result
    ) {
        if (!result.accepted) {
            std::cout
                << "REJECTED: "
                << rejectReasonText(result.rejectReason)
                << '\n';

            return;
        }

        std::cout << "ACCEPTED";

        if (result.restingQuantity > 0) {
            std::cout
                << " | Resting quantity: "
                << result.restingQuantity;
        }

        std::cout << '\n';

        for (const lob::Trade& trade : result.trades) {
            std::cout
                << "TRADE"
                << " | ID: " << trade.tradeId
                << " | Maker: " << trade.makerOrderId
                << " | Taker: " << trade.takerOrderId
                << " | Side: "
                << sideText(trade.aggressorSide)
                << " | Price: "
                << formatPrice(trade.price)
                << " | Quantity: "
                << trade.quantity
                << '\n';
        }
    }

    void printCancelResult(
        const lob::CancelResult& result
    ) {
        if (result.cancelled) {
            std::cout << "ORDER CANCELLED\n";
            return;
        }

        std::cout
            << "CANCELLATION REJECTED: "
            << rejectReasonText(result.rejectReason)
            << '\n';
    }

    void printBook(
        const lob::MatchingEngine& engine,
        const lob::BookCommand& command
    ) {
        const lob::OrderBook* book =
            engine.findBook(command.symbol);

        if (book == nullptr) {
            std::cout
                << "No order book exists for "
                << command.symbol
                << ".\n";

            return;
        }

        const auto asks =
            book->askDepth(command.levels);

        const auto bids =
            book->bidDepth(command.levels);

        std::cout
            << "\nORDER BOOK: "
            << command.symbol
            << '\n';

        std::cout << "ASKS (best first)\n";

        if (asks.empty()) {
            std::cout << "  Empty\n";
        }

        for (const lob::LevelView& level : asks) {
            std::cout
                << "  Price: " << formatPrice(level.price)
                << " | Quantity: " << level.totalQuantity
                << " | Orders: " << level.orderCount
                << '\n';
        }

        std::cout << "BIDS (best first)\n";

        if (bids.empty()) {
            std::cout << "  Empty\n";
        }

        for (const lob::LevelView& level : bids) {
            std::cout
                << "  Price: " << formatPrice(level.price)
                << " | Quantity: " << level.totalQuantity
                << " | Orders: " << level.orderCount
                << '\n';
        }

        const auto spread = book->spread();

        if (spread.has_value()) {
            std::cout
                << "Spread: "
                << formatPrice(*spread)
                << '\n';
        }

        std::cout
            << "Resting orders: "
            << book->orderCount()
            << "\n\n";
    }

    void printInstructions() {
        std::cout
            << "PriceTime Matching Engine\n\n"
            << "Commands:\n"
            << "  ADD symbol id BUY/SELL price quantity\n"
            << "  CANCEL symbol id\n"
            << "  MODIFY symbol id newPrice newQuantity\n"
            << "  BOOK symbol levels\n"
            << "  QUIT\n\n"
            << "Prices use integer ticks: "
            << "1050 represents 10.50.\n\n";
    }

} // anonymous namespace

int main() {
    lob::MatchingEngine engine;
    std::string line;

    printInstructions();

    while (true) {
        std::cout << "> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        lob::ParseResult parsed =
            lob::parseCommand(line);

        if (!parsed.succeeded()) {
            std::cout
                << "ERROR: "
                << parsed.error
                << '\n';

            continue;
        }

        const lob::Command& command =
            *parsed.command;

        if (const auto* submit =
            std::get_if<lob::SubmitCommand>(
                &command
            )) {
            const lob::ExecutionResult result =
                engine.submit(
                    submit->symbol,
                    submit->order
                );

            printExecutionResult(result);
            continue;
        }

        if (const auto* cancellation =
            std::get_if<lob::CancelCommand>(
                &command
            )) {
            const lob::CancelResult result =
                engine.cancel(
                    cancellation->symbol,
                    cancellation->cancellation.id
                );

            printCancelResult(result);
            continue;
        }

        if (const auto* modification =
            std::get_if<lob::ModifyCommand>(
                &command
            )) {
            const lob::ExecutionResult result =
                engine.modify(
                    modification->symbol,
                    modification->modification
                );

            printExecutionResult(result);
            continue;
        }

        if (const auto* book =
            std::get_if<lob::BookCommand>(
                &command
            )) {
            printBook(engine, *book);
            continue;
        }

        if (std::holds_alternative<lob::QuitCommand>(
            command
        )) {
            std::cout << "PriceTime closed.\n";
            break;
        }
    }

    return 0;
}