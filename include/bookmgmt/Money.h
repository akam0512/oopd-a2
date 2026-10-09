#pragma once
// Money: fixed-point currency amount stored in minor units (e.g. paise/cents).
// Using an integer avoids floating-point rounding errors in cost/budget sums.
//
// Q15: every amount has a currency code (three letters, such as "INR" or "USD").
// An amount made without a currency is in INR. Adding, subtracting or comparing
// amounts in different currencies throws CurrencyMismatchError.
// The one exception is zero: a zero amount can be combined with any currency, so
// a running total that starts as Money{} works in whatever currency is added to it.

#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>

namespace bookmgmt
{

    class Money
    {
    public:
        static constexpr const char *kDefaultCurrency = "INR";

        Money() = default; // zero, in the default currency

        // Construct from minor units: Money::fromMinor(12550) == 125.50
        // Throws std::invalid_argument if the currency is not three letters.
        static Money fromMinor(std::int64_t minor, const std::string &currency = kDefaultCurrency);
        // Construct from major + minor parts: Money::of(125, 50) == 125.50
        static Money of(std::int64_t major, int minor = 0,
                        const std::string &currency = kDefaultCurrency);

        // The currency code in upper case: "INR", "USD", ...
        const std::string &currency() const { return currency_; }
        std::int64_t minorUnits() const { return minor_; }
        double toDouble() const { return static_cast<double>(minor_) / 100.0; }
        std::string toString() const;             // "1234.50" / "-3.05" (no currency code)
        std::string toStringWithCurrency() const; // "1234.50 INR"
        Money scaledByPercent(int percent) const;

        bool isZero() const { return minor_ == 0; }
        bool isNegative() const { return minor_ < 0; }

        // Adding or subtracting amounts in different currencies throws
        // CurrencyMismatchError (and changes nothing), unless one of them is zero.
        Money &operator+=(Money o);
        Money &operator-=(Money o);
        Money &operator*=(std::int64_t k)
        {
            minor_ *= k;
            return *this;
        }

        friend Money operator+(Money a, Money b) { return a += b; }
        friend Money operator-(Money a, Money b) { return a -= b; }
        friend Money operator*(Money a, std::int64_t k) { return a *= k; }
        friend Money operator*(std::int64_t k, Money a) { return a *= k; }

        // Comparing amounts in different currencies throws CurrencyMismatchError,
        // unless one of them is zero.
        friend bool operator==(Money a, Money b);
        friend bool operator!=(Money a, Money b);
        friend bool operator<(Money a, Money b);
        friend bool operator<=(Money a, Money b);
        friend bool operator>(Money a, Money b);
        friend bool operator>=(Money a, Money b);

    private:
        Money(std::int64_t minor, std::string currency)
            : minor_(minor), currency_(std::move(currency)) {}

        // Checks that the code is three letters; returns it in upper case.
        static std::string checkedCurrency(const std::string &code);
        // Before adding or subtracting `o`: makes sure the currencies fit.
        void matchCurrencyOf(const Money &o);
        // Before comparing: throws if the currencies differ and neither is zero.
        static void requireComparable(const Money &a, const Money &b);

        std::int64_t minor_ = 0;
        std::string currency_ = kDefaultCurrency;
    };

    std::ostream &operator<<(std::ostream &os, Money m); // prints the amount only

} // namespace bookmgmt