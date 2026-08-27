#pragma once

#include "Order.hpp"

#include <stdexcept>


namespace lob {
	Order::Order(
		OrderId id,
		Side side,
		Price price,
		Quantity quantity,
		Sequence sequence
	) : id_(id), side_(side), price_(price), originalQuantity_(quantity), remainingQuantity_(quantity), sequence_(sequence) {
		if (quantity == 0) {
			throw std::invalid_argument("Quantity must be greater than zero.");
		}
	}
	OrderId Order::id() const noexcept {
		return id_;
	}
	Side Order::side() const noexcept {
		return side_;
	}
	Price Order::price() const noexcept {
		return price_;
	}
	Quantity Order::originalQuantity() const noexcept {
		return originalQuantity_;
	}
	Quantity Order::remainingQuantity() const noexcept {
		return remainingQuantity_;
	}
	Sequence Order::sequence() const noexcept {
		return sequence_;
	}
	bool Order::isFilled() const noexcept {
		return remainingQuantity_ == 0;
	}
	void Order::reduce(Quantity amount) {
		if (amount > remainingQuantity_) {
			throw std::invalid_argument("Reduction amount exceeds remaining quantity.");
		}
		remainingQuantity_ -= amount;
	}
} // namespace lob}