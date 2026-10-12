#pragma once
// Q6: configurable tax rates. Print items and electronic items can be taxed at
// different rates (whole percent). Both rates default to 0, so code that does
// not configure taxes behaves exactly as before.

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt
{

    class TaxPolicy
    {
    public:
        TaxPolicy(int printPercent = 0, int electronicPercent = 0);

        int printPercent() const { return printPercent_; }
        int electronicPercent() const { return electronicPercent_; }
        void setPrintPercent(int percent);
        void setElectronicPercent(int percent);

        // Rate that applies to a resource: electronic (digital) or print.
        int rateFor(const Resource &r) const;
        // Tax on a pre-tax amount for this resource, rounded half away from zero.
        Money taxOn(const Resource &r, Money preTax) const;

    private:
        static void requireValid(int percent);

        int printPercent_;
        int electronicPercent_;
    };

} // namespace bookmgmt