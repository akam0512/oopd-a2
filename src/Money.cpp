// MT26014-Akanksha Mishra
#include "bookmgmt/Money.h"

#include <cctype>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    std::string Money::checkedCurrency(const std::string &code)
    {
        if (code.size() != 3)
            throw std::invalid_argument("currency code must be three letters: \"" + code + "\"");
        std::string upper;
        for (unsigned char ch : code)
        {
            if (!std::isalpha(ch))
                throw std::invalid_argument("currency code must be three letters: \"" + code + "\"");
            upper += static_cast<char>(std::toupper(ch));
        }
        return upper;
    }

    Money Money::fromMinor(std::int64_t minor, const std::string &currency)
    {
        return Money(minor, checkedCurrency(currency));
    }

    Money Money::of(std::int64_t major, int minor, const std::string &currency)
    {
        if (minor < 0 || minor > 99)
            throw std::invalid_argument("minor part must be in 0..99");
        const std::int64_t sign = major < 0 ? -1 : 1;
        return Money(major * 100 + sign * minor, checkedCurrency(currency));
    }

    std::string Money::toString() const
    {
        const std::int64_t absMinor = minor_ < 0 ? -minor_ : minor_;
        std::string s = std::to_string(absMinor / 100);
        const auto frac = absMinor % 100;
        s += '.';
        if (frac < 10)
            s += '0';
        s += std::to_string(frac);
        return minor_ < 0 ? "-" + s : s;
    }

    std::string Money::toStringWithCurrency() const { return toString() + " " + currency_; }

    std::ostream &operator<<(std::ostream &os, Money m) { return os << m.toString(); }

    Money Money::scaledByPercent(int percent) const
    {
        const std::int64_t scaled = minor_ * percent;
        return Money((scaled + (scaled >= 0 ? 50 : -50)) / 100, currency_);
    }

    void Money::matchCurrencyOf(const Money &o)
    {
        if (currency_ == o.currency_ || o.isZero())
            return; // same currency, or adding nothing
        if (isZero())
        {
            currency_ = o.currency_; // a zero total takes the currency of what is added
            return;
        }
        throw CurrencyMismatchError(currency_, o.currency_);
    }

    Money &Money::operator+=(Money o)
    {
        matchCurrencyOf(o);
        minor_ += o.minor_;
        return *this;
    }

    Money &Money::operator-=(Money o)
    {
        matchCurrencyOf(o);
        minor_ -= o.minor_;
        return *this;
    }

    void Money::requireComparable(const Money &a, const Money &b)
    {
        if (a.currency_ != b.currency_ && !a.isZero() && !b.isZero())
            throw CurrencyMismatchError(a.currency_, b.currency_);
    }

    bool operator==(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ == b.minor_;
    }

    bool operator!=(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ != b.minor_;
    }

    bool operator<(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ < b.minor_;
    }

    bool operator<=(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ <= b.minor_;
    }

    bool operator>(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ > b.minor_;
    }

    bool operator>=(Money a, Money b)
    {
        Money::requireComparable(a, b);
        return a.minor_ >= b.minor_;
    }

} // namespace bookmgmt