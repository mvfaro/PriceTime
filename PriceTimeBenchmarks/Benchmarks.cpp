#include "OrderBook.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>

namespace {

    constexpr lob::Price kPrice{ 1000 };
    constexpr lob::Quantity kQuantity{ 100 };

    void BM_NonCrossingInsertion(
        benchmark::State& state
    ) {
        lob::OrderBook book;
        lob::OrderId nextOrderId{ 1 };

        for (auto _ : state) {
            (void)_;

            state.PauseTiming();

            const lob::OrderId orderId =
                nextOrderId++;

            const lob::NewOrder order{
                orderId,
                lob::Side::Buy,
                kPrice,
                kQuantity
            };

            state.ResumeTiming();

            {
                lob::ExecutionResult result =
                    book.submit(order);

                benchmark::DoNotOptimize(
                    result.accepted
                );
            }

            // Remove the order without measuring cleanup.
            state.PauseTiming();

            {
                lob::CancelResult cleanup =
                    book.cancel(orderId);

                benchmark::DoNotOptimize(
                    cleanup.cancelled
                );
            }

            state.ResumeTiming();
        }

        state.SetItemsProcessed(
            state.iterations()
        );
    }

    void BM_CancelRestingOrder(
        benchmark::State& state
    ) {
        lob::OrderBook book;
        lob::OrderId nextOrderId{ 1 };

        for (auto _ : state) {
            (void)_;

            // Add the order without measuring setup.
            state.PauseTiming();

            const lob::OrderId orderId =
                nextOrderId++;

            {
                lob::ExecutionResult setup =
                    book.submit(
                        lob::NewOrder{
                            orderId,
                            lob::Side::Buy,
                            kPrice,
                            kQuantity
                        }
                    );

                benchmark::DoNotOptimize(
                    setup.accepted
                );
            }

            state.ResumeTiming();

            lob::CancelResult result =
                book.cancel(orderId);

            benchmark::DoNotOptimize(
                result.cancelled
            );
        }

        state.SetItemsProcessed(
            state.iterations()
        );
    }

    void BM_ImmediateMatch(
        benchmark::State& state
    ) {
        lob::OrderBook book;
        lob::OrderId nextOrderId{ 1 };

        for (auto _ : state) {
            (void)_;

            // Install the resting sell without measuring setup.
            state.PauseTiming();

            const lob::OrderId makerOrderId =
                nextOrderId++;

            const lob::OrderId takerOrderId =
                nextOrderId++;

            {
                lob::ExecutionResult setup =
                    book.submit(
                        lob::NewOrder{
                            makerOrderId,
                            lob::Side::Sell,
                            kPrice,
                            kQuantity
                        }
                    );

                benchmark::DoNotOptimize(
                    setup.accepted
                );
            }

            const lob::NewOrder incoming{
                takerOrderId,
                lob::Side::Buy,
                kPrice,
                kQuantity
            };

            state.ResumeTiming();

            lob::ExecutionResult result =
                book.submit(incoming);

            std::size_t tradeCount =
                result.trades.size();

            benchmark::DoNotOptimize(
                tradeCount
            );
        }

        state.SetItemsProcessed(
            state.iterations()
        );
    }

    void BM_SweepAskLevels(
        benchmark::State& state
    ) {
        lob::OrderBook book;
        lob::OrderId nextOrderId{ 1 };

        const std::size_t levelCount =
            static_cast<std::size_t>(
                state.range(0)
                );

        for (auto _ : state) {
            (void)_;

            // Create the ask levels without measuring setup.
            state.PauseTiming();

            for (
                std::size_t level = 0;
                level < levelCount;
                ++level
                ) {
                const lob::Price price =
                    static_cast<lob::Price>(
                        kPrice + level
                        );

                lob::ExecutionResult setup =
                    book.submit(
                        lob::NewOrder{
                            nextOrderId++,
                            lob::Side::Sell,
                            price,
                            10
                        }
                    );

                benchmark::DoNotOptimize(
                    setup.accepted
                );
            }

            const lob::OrderId takerOrderId =
                nextOrderId++;

            const lob::Price highestPrice =
                static_cast<lob::Price>(
                    kPrice + levelCount - 1
                    );

            const lob::Quantity sweepQuantity =
                static_cast<lob::Quantity>(
                    levelCount * 10
                    );

            const lob::NewOrder incoming{
                takerOrderId,
                lob::Side::Buy,
                highestPrice,
                sweepQuantity
            };

            state.ResumeTiming();

            lob::ExecutionResult result =
                book.submit(incoming);

            std::size_t tradeCount =
                result.trades.size();

            benchmark::DoNotOptimize(
                tradeCount
            );
        }

        state.SetItemsProcessed(
            state.iterations()
        );
    }

    void BM_MixedWorkload(
        benchmark::State& state
    ) {
        constexpr std::size_t buyCount{ 32 };
        constexpr std::size_t cancelCount{ 8 };
        constexpr std::size_t sellCount{
            buyCount - cancelCount
        };

        lob::OrderBook book;
        lob::OrderId nextOrderId{ 1 };

        std::array<
            lob::OrderId,
            buyCount
        > buyOrderIds{};

        for (auto _ : state) {
            (void)_;

            // Add 32 resting buy orders.
            for (
                std::size_t index = 0;
                index < buyCount;
                ++index
                ) {
                const lob::OrderId orderId =
                    nextOrderId++;

                buyOrderIds[index] = orderId;

                lob::ExecutionResult result =
                    book.submit(
                        lob::NewOrder{
                            orderId,
                            lob::Side::Buy,
                            kPrice,
                            kQuantity
                        }
                    );

                benchmark::DoNotOptimize(
                    result.accepted
                );
            }

            // Cancel eight of those orders.
            for (
                std::size_t index = 0;
                index < cancelCount;
                ++index
                ) {
                lob::CancelResult result =
                    book.cancel(
                        buyOrderIds[index]
                    );

                benchmark::DoNotOptimize(
                    result.cancelled
                );
            }

            // Match the remaining 24 buy orders.
            for (
                std::size_t index = 0;
                index < sellCount;
                ++index
                ) {
                lob::ExecutionResult result =
                    book.submit(
                        lob::NewOrder{
                            nextOrderId++,
                            lob::Side::Sell,
                            kPrice,
                            kQuantity
                        }
                    );

                std::size_t tradeCount =
                    result.trades.size();

                benchmark::DoNotOptimize(
                    tradeCount
                );
            }

            std::size_t remainingOrders =
                book.orderCount();

            benchmark::DoNotOptimize(
                remainingOrders
            );

            benchmark::ClobberMemory();
        }

        // 32 submissions + 8 cancellations
        // + 24 matching submissions.
        state.SetItemsProcessed(
            state.iterations() * 64
        );
    }

    BENCHMARK(BM_NonCrossingInsertion)
        ->Unit(benchmark::kNanosecond);

    BENCHMARK(BM_CancelRestingOrder)
        ->Unit(benchmark::kNanosecond);

    BENCHMARK(BM_ImmediateMatch)
        ->Unit(benchmark::kNanosecond);

    BENCHMARK(BM_SweepAskLevels)
        ->Arg(1)
        ->Arg(5)
        ->Arg(10)
        ->Arg(50)
        ->Unit(benchmark::kNanosecond);

    BENCHMARK(BM_MixedWorkload)
        ->Unit(benchmark::kNanosecond);

} // anonymous namespace

BENCHMARK_MAIN();