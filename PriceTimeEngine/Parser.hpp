#pragma once

#include "Commands.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace lob {

    struct ParseResult {
        std::optional<Command> command;
        std::string error;

        [[nodiscard]] bool succeeded() const noexcept {
            return command.has_value();
        }
    };

    [[nodiscard]] ParseResult parseCommand(
        std::string_view line
    );

} // namespace lob
