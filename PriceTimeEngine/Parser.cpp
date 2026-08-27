#include "Parser.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace lob {

    namespace {

        std::vector<std::string> tokenize(
            std::string_view line
        ) {
            std::istringstream input{ std::string(line) };
            std::vector<std::string> tokens;
            std::string token;

            while (input >> token) {
                tokens.push_back(std::move(token));
            }

            return tokens;
        }

        std::string toUpper(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char character) {
                    return static_cast<char>(
                        std::toupper(character)
                        );
                }
            );

            return value;
        }

        template<typename Number>
        bool parseNumber(
            const std::string& text,
            Number& value
        ) {
            const char* begin = text.data();
            const char* end = begin + text.size();

            const auto [position, error] =
                std::from_chars(begin, end, value);

            return error == std::errc{} &&
                position == end;
        }

        ParseResult failure(std::string message) {
            return {
                std::nullopt,
                std::move(message)
            };
        }

        ParseResult success(Command command) {
            return {
                std::move(command),
                {}
            };
        }

    } // anonymous namespace

    ParseResult parseCommand(std::string_view line) {
        const std::vector<std::string> tokens =
            tokenize(line);

        if (tokens.empty()) {
            return failure("Command cannot be empty.");
        }

        const std::string action = toUpper(tokens[0]);

        if (action == "ADD") {
            if (tokens.size() != 6) {
                return failure(
                    "Usage: ADD symbol id BUY/SELL "
                    "price quantity"
                );
            }

            OrderId orderId;
            Price price;
            Quantity quantity;

            if (!parseNumber(tokens[2], orderId)) {
                return failure("Invalid order ID.");
            }

            const std::string sideText =
                toUpper(tokens[3]);

            Side side;

            if (sideText == "BUY") {
                side = Side::Buy;
            }
            else if (sideText == "SELL") {
                side = Side::Sell;
            }
            else {
                return failure(
                    "Side must be BUY or SELL."
                );
            }

            if (!parseNumber(tokens[4], price) ||
                price <= 0) {
                return failure(
                    "Price must be a positive integer."
                );
            }

            if (!parseNumber(tokens[5], quantity) ||
                quantity == 0) {
                return failure(
                    "Quantity must be greater than zero."
                );
            }

            return success(
                Command{
                    SubmitCommand{
                        tokens[1],
                        NewOrder{
                            orderId,
                            side,
                            price,
                            quantity
                        }
                    }
                }
            );
        }

        if (action == "CANCEL") {
            if (tokens.size() != 3) {
                return failure(
                    "Usage: CANCEL symbol id"
                );
            }

            OrderId orderId;

            if (!parseNumber(tokens[2], orderId)) {
                return failure("Invalid order ID.");
            }

            return success(
                Command{
                    CancelCommand{
                        tokens[1],
                        CancelOrder{orderId}
                    }
                }
            );
        }

        if (action == "MODIFY") {
            if (tokens.size() != 5) {
                return failure(
                    "Usage: MODIFY symbol id "
                    "newPrice newQuantity"
                );
            }

            OrderId orderId;
            Price newPrice;
            Quantity newQuantity;

            if (!parseNumber(tokens[2], orderId)) {
                return failure("Invalid order ID.");
            }

            if (!parseNumber(tokens[3], newPrice) ||
                newPrice <= 0) {
                return failure(
                    "New price must be positive."
                );
            }

            if (!parseNumber(tokens[4], newQuantity) ||
                newQuantity == 0) {
                return failure(
                    "New quantity must be greater than zero."
                );
            }

            return success(
                Command{
                    ModifyCommand{
                        tokens[1],
                        ModifyOrder{
                            orderId,
                            newPrice,
                            newQuantity
                        }
                    }
                }
            );
        }

        if (action == "BOOK") {
            if (tokens.size() != 3) {
                return failure(
                    "Usage: BOOK symbol levels"
                );
            }

            std::size_t levels;

            if (!parseNumber(tokens[2], levels) ||
                levels == 0) {
                return failure(
                    "Levels must be greater than zero."
                );
            }

            return success(
                Command{
                    BookCommand{
                        tokens[1],
                        levels
                    }
                }
            );
        }

        if (action == "QUIT") {
            if (tokens.size() != 1) {
                return failure(
                    "QUIT does not accept arguments."
                );
            }

            return success(Command{ QuitCommand{} });
        }

        return failure("Unknown command.");
    }

} // namespace lob