#include "bookmgmt/Money.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

Money Money::of(std::int64_t major, int minor) {
    if (minor < 0 || minor > 99)
        throw std::invalid_argument("minor part must be in 0..99");
    const std::int64_t sign = major < 0 ? -1 : 1;
    return Money(major * 100 + sign * minor);
}

std::string Money::toString() const {
    const std::int64_t absMinor = minor_ < 0 ? -minor_ : minor_;
    std::string s = std::to_string(absMinor / 100);
    const auto frac = absMinor % 100;
    s += '.';
    if (frac < 10) s += '0';
    s += std::to_string(frac);
    return minor_ < 0 ? "-" + s : s;
}

std::ostream& operator<<(std::ostream& os, Money m) { return os << m.toString(); }

}  // namespace bookmgmt
