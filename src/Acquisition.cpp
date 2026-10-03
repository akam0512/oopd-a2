#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    AcquisitionManager::AcquisitionManager(Catalog &catalog, Budget &budget, TaxPolicy taxPolicy)
        : catalog_(catalog), budget_(budget), taxPolicy_(taxPolicy) {}

    AcquisitionManager::Pricing AcquisitionManager::pricing(const Resource &r,
                                                            int quantity) const
    {
        const Money preTax = r.costFor(quantity); // may throw invalid_argument
        return Pricing{preTax, taxPolicy_.taxOn(r, preTax)};
    }

    Money AcquisitionManager::quote(const std::string &id, int quantity) const
    {
        return catalog_.get(id).costFor(quantity);
    }

    Money AcquisitionManager::quoteWithTax(const std::string &id, int quantity) const
    {
        return pricing(catalog_.get(id), quantity).total();
    }

    bool AcquisitionManager::canPurchase(const std::string &id, int quantity,
                                         std::string *reason) const
    {
        std::string why;
        if (const Resource *r = catalog_.find(id))
        {
            if (quantity <= 0)
                why = "quantity must be positive";
            else
                why = budget_.check(r->category(), quantity, pricing(*r, quantity).total());
        }
        else
        {
            why = "resource not found: " + id;
        }
        if (reason)
            *reason = why;
        return why.empty();
    }

    PurchaseRecord &AcquisitionManager::record(const Resource *r, const std::string &id,
                                               int qty, Pricing price, bool approved,
                                               std::string reason)
    {
        history_.push_back(PurchaseRecord{
            nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
            r ? r->category() : ResourceCategory::Book, qty, price.total(), approved,
            std::move(reason), price.preTax, price.tax});
        return history_.back();
    }

    const PurchaseRecord &AcquisitionManager::purchase(const std::string &id, int quantity)
    {
        const Resource &r = catalog_.get(id);       // may throw NotFoundError
        const Pricing price = pricing(r, quantity); // may throw invalid_argument
        // Quotas and the budget are checked against the post-tax cost.
        budget_.commit(r.category(), quantity, price.total()); // may throw quota/budget errors
        catalog_.addHoldings(id, quantity);
        return record(&r, id, quantity, price, true, {});
    }

    std::vector<PurchaseRecord> AcquisitionManager::processBatch(
        const std::vector<PurchaseRequest> &reqs)
    {
        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());
        for (const auto &req : reqs)
        {
            const Resource *r = catalog_.find(req.resourceId);
            Pricing price{};
            std::string why;
            if (!r)
            {
                why = "resource not found: " + req.resourceId;
            }
            else if (req.quantity <= 0)
            {
                why = "quantity must be positive";
            }
            else
            {
                price = pricing(*r, req.quantity);
                why = budget_.check(r->category(), req.quantity, price.total());
            }

            if (why.empty())
            {
                results.push_back(purchase(req.resourceId, req.quantity));
            }
            else
            {
                results.push_back(record(r, req.resourceId, req.quantity, price, false, why));
            }
        }
        return results;
    }

    Money AcquisitionManager::totalSpent() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (rec.approved)
                sum += rec.cost;
        return sum;
    }

    Money AcquisitionManager::totalSpentPreTax() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (rec.approved)
                sum += rec.preTaxCost;
        return sum;
    }

    Money AcquisitionManager::totalTax() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (rec.approved)
                sum += rec.tax;
        return sum;
    }

    void AcquisitionManager::printReport(std::ostream &os) const
    {
        os << "Order history (" << history_.size() << " orders)\n";
        for (const auto &rec : history_)
        {
            os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
               << (rec.approved ? "APPROVED" : "REJECTED") << "  " << std::setw(6)
               << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
               << std::setw(12) << std::right << rec.cost.toString() << std::left << "  "
               << rec.title;
            if (!rec.approved)
                os << "\n        reason: " << rec.reason;
            os << "\n";
        }
        os << "Total pre-tax: " << totalSpentPreTax() << "\n";
        os << "Total tax: " << totalTax() << "\n";
        os << "Total spent (post-tax): " << totalSpent() << "\n";
    }

} // namespace bookmgmt