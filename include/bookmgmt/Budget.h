#pragma once
// Budget: an overall spending limit plus optional per-category purchase quotas.
//
// A quota caps how many units (copies/seats) and how much money may be spent on
// one category, and (Q7) how many different titles may be bought in it.
// Categories with no quota are limited only by the total budget.

#include <iosfwd>
#include <limits>
#include <map>
#include <optional>
#include <string>

#include "bookmgmt/Money.h"
#include "bookmgmt/Resource.h"

namespace bookmgmt
{

    struct Quota
    {
        static constexpr int kNoLimit = std::numeric_limits<int>::max();

        int maxUnits;             // maximum copies/seats that may be bought
        Money maxSpend;           // maximum money that may be spent
        int maxTitles = kNoLimit; // Q7: maximum number of different titles
    };

    struct Usage
    {
        int units = 0;
        Money spent;
        std::map<std::string, int> unitsByTitle; // Q7: title id -> units bought

        // Number of different titles bought in the category.
        int titles() const { return static_cast<int>(unitsByTitle.size()); }
    };

    class Budget
    {
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
        // Q7: nullopt means "no quota set" or "no title limit in the quota".
        std::optional<int> titlesRemaining(ResourceCategory c) const;

        // Returns an empty string if the purchase fits, otherwise the reason it
        // does not. Does not change state. Checks run in this order: input, unit
        // quota, title quota, spend quota, overall budget.
        // `resourceId` names the title being bought; if it is empty, the title
        // limit is not checked.
        std::string check(ResourceCategory c, int units, Money cost,
                          const std::string &resourceId = {}) const;

        // Records a purchase. Throws QuotaExceededError / BudgetExceededError
        // (and changes nothing) if it would not fit.
        void commit(ResourceCategory c, int units, Money cost,
                    const std::string &resourceId = {});

        void print(std::ostream &os) const;

    private:
        enum class Failure
        {
            None,
            BadInput,
            Quota,
            Overall
        };
        Failure evaluate(ResourceCategory c, int units, Money cost,
                         const std::string &resourceId, std::string &why) const;

        Money total_;
        Money spent_;
        std::map<ResourceCategory, Quota> quotas_;
        std::map<ResourceCategory, Usage> usage_;
    };

} // namespace bookmgmt