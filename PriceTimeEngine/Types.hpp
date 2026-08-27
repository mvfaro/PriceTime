// Types.hpp is a header file that defines various types used in the PriceTimeEngine project. It includes type definitions, enumerations, and possibly some utility functions or classes that are relevant to the project's domain. The use of `#pragma once` ensures that the header file is included only once during compilation, preventing multiple definition errors.
#pragma once

#include <cstdint>

namespace lob{

	using OrderId = std::uint64_t;
	using TradeId = std::uint64_t;
	using Price = std::uint64_t;
	using Quantity = std::uint64_t;
	using Sequence = std::uint64_t;

	enum class Side : std::uint8_t {
		Buy,
		Sell

	};

	enum class RejectReason : std::uint8_t {
		None,
		DuplicateOrderId,
		UnknownOrderId,
		InvalidPrice,
		InvalidQuantity,
	};

} // namespace lob.
