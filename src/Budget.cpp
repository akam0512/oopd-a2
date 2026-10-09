#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    namespace
    {
        // Every category, in the order Budget::print() lists them.
        const ResourceCategory kAllCategories[] = {ResourceCategory::Book,
                                                   ResourceCategory::ElectronicResource,
                                                   ResourceCategory::Journal,
                                                   ResourceCategory::EBook,
                                                   ResourceCategory::AudioBook,
                                                   ResourceCategory::Thesis};
    }

    Budget::Budget(Money total) : total_(total)
    {
        if (total_.isNegative())
            throw std::invalid_argument("budget must not be negative");
    }

    void Budget::setQuota(ResourceCategory c, Quota q)
    {
        if (q.maxUnits < 0 || q.maxSpend.isNegative() || q.maxTitles < 0)
            throw std::invalid_argument("quota limits must not be negative");
        quotas_[c] = q;
    }

    void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

    std::optional<Quota> Budget::quotaFor(ResourceCategory c) const
    {
        auto it = quotas_.find(c);
        if (it == quotas_.end())
            return std::nullopt;
        return it->second;
    }

    Usage Budget::usageFor(ResourceCategory c) const
    {
        auto it = usage_.find(c);
        return it == usage_.end() ? Usage{} : it->second;
    }

    std::optional<int> Budget::unitsRemaining(ResourceCategory c) const
    {
        auto q = quotaFor(c);
        if (!q)
            return std::nullopt;
        return q->maxUnits - usageFor(c).units;
    }

    std::optional<Money> Budget::spendRemaining(ResourceCategory c) const
    {
        auto q = quotaFor(c);
        if (!q)
            return std::nullopt;
        return q->maxSpend - usageFor(c).spent;
    }

    std::optional<int> Budget::titlesRemaining(ResourceCategory c) const
    {
        auto q = quotaFor(c);
        if (!q || q->maxTitles == Quota::kNoLimit)
            return std::nullopt;
        return q->maxTitles - usageFor(c).titles();
    }

    Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                     const std::string &resourceId, std::string &why) const
    {
        try
        {
            if (units <= 0)
            {
                why = "quantity must be positive";
                return Failure::BadInput;
            }
            if (cost.isNegative())
            {
                why = "cost must not be negative";
                return Failure::BadInput;
            }
            if (auto left = unitsRemaining(c); left && units > *left)
            {
                why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
                      std::to_string(units) + ", " + std::to_string(*left) + " remaining";
                return Failure::Quota;
            }
            // Q7: buying more of a title already bought is always fine; only a new
            // title counts against the limit.
            if (auto left = titlesRemaining(c); left && !resourceId.empty())
            {
                const Usage u = usageFor(c);
                const bool isNewTitle = u.unitsByTitle.find(resourceId) == u.unitsByTitle.end();
                if (isNewTitle && *left <= 0)
                {
                    why = std::string(categoryName(c)) + " title quota exceeded: " + resourceId +
                          " would be a new title, " + std::to_string(u.titles()) + " of " +
                          std::to_string(quotaFor(c)->maxTitles) + " already bought";
                    return Failure::Quota;
                }
            }
            if (auto left = spendRemaining(c); left && cost > *left)
            {
                why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
                      cost.toString() + ", " + left->toString() + " remaining";
                return Failure::Quota;
            }
            if (cost > remaining())
            {
                why = "overall budget exceeded: cost " + cost.toString() + ", " +
                      remaining().toString() + " remaining";
                return Failure::Overall;
            }
            why.clear();
            return Failure::None;
        }
        catch (const CurrencyMismatchError &e)
        {
            // Q15: a price, quota or budget in another currency cannot be compared.
            why = e.what();
            return Failure::BadInput;
        }
    }

    std::string Budget::check(ResourceCategory c, int units, Money cost,
                              const std::string &resourceId) const
    {
        std::string why;
        evaluate(c, units, cost, resourceId, why);
        return why;
    }

    void Budget::commit(ResourceCategory c, int units, Money cost,
                        const std::string &resourceId)
    {
        std::string why;
        switch (evaluate(c, units, cost, resourceId, why))
        {
        case Failure::None:
            break;
        case Failure::BadInput:
            throw std::invalid_argument(why);
        case Failure::Quota:
            throw QuotaExceededError(why);
        case Failure::Overall:
            throw BudgetExceededError(why);
        }
        Usage &u = usage_[c];
        u.units += units;
        u.spent += cost;
        if (!resourceId.empty())
            u.unitsByTitle[resourceId] += units;
        spent_ += cost;
    }
    void Budget::refund(ResourceCategory c, int units, Money cost,
                        const std::string &resourceId)
    {
        if (units <= 0)
            throw std::invalid_argument("refund quantity must be positive");
        if (cost.isNegative())
            throw std::invalid_argument("refund amount must not be negative");

        // Check everything first, so a failed refund changes nothing.
        auto usageIt = usage_.find(c);
        if (usageIt == usage_.end() || units > usageIt->second.units ||
            cost > usageIt->second.spent)
            throw std::invalid_argument(std::string("refund is larger than what was bought in ") +
                                        categoryName(c));

        Usage &u = usageIt->second;
        auto titleIt = u.unitsByTitle.end();
        if (!resourceId.empty())
        {
            titleIt = u.unitsByTitle.find(resourceId);
            if (titleIt == u.unitsByTitle.end() || units > titleIt->second)
                throw std::invalid_argument("refund is larger than what was bought of " +
                                            resourceId);
        }

        u.units -= units;
        u.spent -= cost;
        spent_ -= cost;
        if (titleIt != u.unitsByTitle.end())
        {
            titleIt->second -= units;
            if (titleIt->second == 0)
                u.unitsByTitle.erase(titleIt); // title no longer held
        }
    }
    Money Budget::carryForwardAmount(int carryForwardPercent) const
    {
        if (carryForwardPercent < 0 || carryForwardPercent > 100)
            throw std::invalid_argument("carry-forward percentage must be between 0 and 100");
        return remaining().scaledByPercent(carryForwardPercent);
    }

    Budget Budget::rollover(int carryForwardPercent, Money newAllocation) const
    {
        if (newAllocation.isNegative())
            throw std::invalid_argument("new allocation must not be negative");
        Budget next(newAllocation + carryForwardAmount(carryForwardPercent));
        next.quotas_ = quotas_; // same limits; usage starts at zero
        return next;
    }

    Budget Budget::rollover(int carryForwardPercent) const
    {
        return rollover(carryForwardPercent, total_);
    }
    void Budget::print(std::ostream &os) const
    {
        os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
           << remaining() << "\n";
        os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
           << std::setw(18) << "Titles used/max" << "Spend used/max\n";
        for (ResourceCategory c : kAllCategories)
        {
            const Usage u = usageFor(c);
            const auto q = quotaFor(c);
            const std::string units =
                std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
            const std::string titles =
                std::to_string(u.titles()) + "/" +
                (q && q->maxTitles != Quota::kNoLimit ? std::to_string(q->maxTitles) : "-");
            const std::string spend =
                u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
            os << "  " << std::setw(20) << categoryName(c) << std::setw(18) << units
               << std::setw(18) << titles << spend << "\n";
        }
    }

} // namespace bookmgmt