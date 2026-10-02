#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents).
// Using an integer avoids floating-point rounding errors in cost/budget sums.

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    constexpr Money() = default;

    // Construct from minor units: Money::fromMinor(12550) == 125.50
    static constexpr Money fromMinor(std::int64_t minor) { return Money(minor); }
    // Construct from major + minor parts: Money::of(125, 50) == 125.50
    static Money of(std::int64_t major, int minor = 0);

    constexpr std::int64_t minorUnits() const { return minor_; }
    double toDouble() const { return static_cast<double>(minor_) / 100.0; }
    std::string toString() const;  // "1234.50" / "-3.05"

    constexpr bool isZero() const { return minor_ == 0; }
    constexpr bool isNegative() const { return minor_ < 0; }

    Money& operator+=(Money o) { minor_ += o.minor_; return *this; }
    Money& operator-=(Money o) { minor_ -= o.minor_; return *this; }
    Money& operator*=(std::int64_t k) { minor_ *= k; return *this; }

    friend Money operator+(Money a, Money b) { return a += b; }
    friend Money operator-(Money a, Money b) { return a -= b; }
    friend Money operator*(Money a, std::int64_t k) { return a *= k; }
    friend Money operator*(std::int64_t k, Money a) { return a *= k; }

    friend constexpr bool operator==(Money a, Money b) { return a.minor_ == b.minor_; }
    friend constexpr bool operator!=(Money a, Money b) { return a.minor_ != b.minor_; }
    friend constexpr bool operator<(Money a, Money b) { return a.minor_ < b.minor_; }
    friend constexpr bool operator<=(Money a, Money b) { return a.minor_ <= b.minor_; }
    friend constexpr bool operator>(Money a, Money b) { return a.minor_ > b.minor_; }
    friend constexpr bool operator>=(Money a, Money b) { return a.minor_ >= b.minor_; }

private:
    explicit constexpr Money(std::int64_t minor) : minor_(minor) {}
    std::int64_t minor_ = 0;
};

std::ostream& operator<<(std::ostream& os, Money m);

}  // namespace bookmgmt
