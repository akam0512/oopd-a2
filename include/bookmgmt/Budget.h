#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category. Categories with no quota are limited only by the total budget.

#include <iosfwd>
#include <map>
#include <optional>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt {

struct Quota {
    int maxUnits;    // maximum copies/seats that may be bought
    Money maxSpend;  // maximum money that may be spent
};

struct Usage {
    int units = 0;
    Money spent;
};

class Budget {
public:
    explicit Budget(Money total);

    Money total() const { return total_; }
    Money spent() const { return spent_; }
    Money remaining() const { return total_ - spent_; }

    void setQuota(ResourceCategory c, Quota q);
    void removeQuota(ResourceCategory c);
    std::optional<Quota> quotaFor(ResourceCategory c) const;
    Usage usageFor(ResourceCategory c) const;

    // Remaining allowance in a category; nullopt means "no quota set".
    std::optional<int> unitsRemaining(ResourceCategory c) const;
    std::optional<Money> spendRemaining(ResourceCategory c) const;

    // Returns an empty string if the purchase fits, otherwise the reason it
    // does not. Does not change state.
    std::string check(ResourceCategory c, int units, Money cost) const;

    // Records a purchase. Throws QuotaExceededError / BudgetExceededError
    // (and changes nothing) if it would not fit.
    void commit(ResourceCategory c, int units, Money cost);

    void print(std::ostream& os) const;

private:
    enum class Failure { None, BadInput, Quota, Overall };
    Failure evaluate(ResourceCategory c, int units, Money cost, std::string& why) const;

    Money total_;
    Money spent_;
    std::map<ResourceCategory, Quota> quotas_;
    std::map<ResourceCategory, Usage> usage_;
};

}  // namespace bookmgmt
