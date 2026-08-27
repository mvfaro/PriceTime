#include "pch.h"
#include "Parser.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <variant>

namespace {

    void expectParseFailure(std::string_view input) {
        const lob::ParseResult result =
            lob::parseCommand(input);

        EXPECT_FALSE(result.succeeded());
        EXPECT_FALSE(result.command.has_value());
        EXPECT_FALSE(result.error.empty());
    }

    TEST(ParserTests, ParsesAddBuyCommand) {
        const lob::ParseResult result =
            lob::parseCommand(
                "ADD AAPL 42 BUY 1050 100"
            );

        ASSERT_TRUE(result.succeeded());

        const auto* command =
            std::get_if<lob::SubmitCommand>(
                &*result.command
            );

        ASSERT_NE(command, nullptr);

        EXPECT_EQ(command->symbol, "AAPL");
        EXPECT_EQ(command->order.id, 42);
        EXPECT_EQ(command->order.side, lob::Side::Buy);
        EXPECT_EQ(command->order.price, 1050);
        EXPECT_EQ(command->order.quantity, 100);
    }

    TEST(ParserTests, ParsesSellCommandCaseInsensitively) {
        const lob::ParseResult result =
            lob::parseCommand(
                "  add MSFT 7 sell 2500 30  "
            );

        ASSERT_TRUE(result.succeeded());

        const auto* command =
            std::get_if<lob::SubmitCommand>(
                &*result.command
            );

        ASSERT_NE(command, nullptr);

        EXPECT_EQ(command->symbol, "MSFT");
        EXPECT_EQ(command->order.id, 7);
        EXPECT_EQ(command->order.side, lob::Side::Sell);
        EXPECT_EQ(command->order.price, 2500);
        EXPECT_EQ(command->order.quantity, 30);
    }

    TEST(ParserTests, ParsesCancelCommand) {
        const lob::ParseResult result =
            lob::parseCommand("CANCEL AAPL 42");

        ASSERT_TRUE(result.succeeded());

        const auto* command =
            std::get_if<lob::CancelCommand>(
                &*result.command
            );

        ASSERT_NE(command, nullptr);

        EXPECT_EQ(command->symbol, "AAPL");
        EXPECT_EQ(command->cancellation.id, 42);
    }

    TEST(ParserTests, ParsesModifyCommand) {
        const lob::ParseResult result =
            lob::parseCommand(
                "MODIFY AAPL 42 1060 80"
            );

        ASSERT_TRUE(result.succeeded());

        const auto* command =
            std::get_if<lob::ModifyCommand>(
                &*result.command
            );

        ASSERT_NE(command, nullptr);

        EXPECT_EQ(command->symbol, "AAPL");
        EXPECT_EQ(command->modification.id, 42);
        EXPECT_EQ(command->modification.newPrice, 1060);
        EXPECT_EQ(
            command->modification.newQuantity,
            80
        );
    }

    TEST(ParserTests, ParsesBookCommand) {
        const lob::ParseResult result =
            lob::parseCommand("BOOK AAPL 5");

        ASSERT_TRUE(result.succeeded());

        const auto* command =
            std::get_if<lob::BookCommand>(
                &*result.command
            );

        ASSERT_NE(command, nullptr);

        EXPECT_EQ(command->symbol, "AAPL");
        EXPECT_EQ(command->levels, 5);
    }

    TEST(ParserTests, ParsesQuitCommand) {
        const lob::ParseResult result =
            lob::parseCommand("QUIT");

        ASSERT_TRUE(result.succeeded());

        EXPECT_TRUE(
            std::holds_alternative<lob::QuitCommand>(
                *result.command
            )
        );
    }

    TEST(ParserTests, RejectsEmptyCommand) {
        expectParseFailure("");
    }

    TEST(ParserTests, RejectsUnknownCommand) {
        expectParseFailure("EXECUTE AAPL");
    }

    TEST(ParserTests, RejectsInvalidSide) {
        expectParseFailure(
            "ADD AAPL 1 HOLD 1050 100"
        );
    }

    TEST(ParserTests, RejectsInvalidOrderId) {
        expectParseFailure(
            "ADD AAPL abc BUY 1050 100"
        );
    }

    TEST(ParserTests, RejectsZeroPrice) {
        expectParseFailure(
            "ADD AAPL 1 BUY 0 100"
        );
    }

    TEST(ParserTests, RejectsZeroQuantity) {
        expectParseFailure(
            "ADD AAPL 1 BUY 1050 0"
        );
    }

    TEST(ParserTests, RejectsIncorrectArgumentCount) {
        expectParseFailure(
            "ADD AAPL 1 BUY 1050"
        );
    }

    TEST(ParserTests, RejectsZeroBookLevels) {
        expectParseFailure("BOOK AAPL 0");
    }

} // anonymous namespace