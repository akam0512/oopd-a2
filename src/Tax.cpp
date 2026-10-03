#include "bookmgmt/Tax.h"

#include <stdexcept>

namespace bookmgmt
{

    void TaxPolicy::requireValid(int percent)
    {
        if (percent < 0 || percent > 100)
            throw std::invalid_argument("tax rate must be between 0 and 100 percent");
    }

    TaxPolicy::TaxPolicy(int printPercent, int electronicPercent)
        : printPercent_(printPercent), electronicPercent_(electronicPercent)
    {
        requireValid(printPercent_);
        requireValid(electronicPercent_);
    }

    void TaxPolicy::setPrintPercent(int percent)
    {
        requireValid(percent);
        printPercent_ = percent;
    }

    void TaxPolicy::setElectronicPercent(int percent)
    {
        requireValid(percent);
        electronicPercent_ = percent;
    }

    int TaxPolicy::rateFor(const Resource &r) const
    {
        return r.isDigital() ? electronicPercent_ : printPercent_;
    }

    Money TaxPolicy::taxOn(const Resource &r, Money preTax) const
    {
        return preTax.scaledByPercent(rateFor(r));
    }

} // namespace bookmgmt