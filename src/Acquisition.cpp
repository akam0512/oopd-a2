#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    namespace
    {
        // Approved orders and cancellations change what was spent (a cancellation
        // counts as negative amounts); rejected orders do not.
        bool affectsTotals(const PurchaseRecord &rec) { return rec.approved || rec.cancellation; }
    } // namespace

    AcquisitionManager::AcquisitionManager(Catalog &catalog, Budget &budget, TaxPolicy taxPolicy)
        : catalog_(catalog), taxPolicy_(taxPolicy)
    {
        budgets_.emplace(Department::general(), &budget);
    }

    AcquisitionManager::AcquisitionManager(Catalog &catalog, TaxPolicy taxPolicy)
        : catalog_(catalog), taxPolicy_(taxPolicy) {}

    void AcquisitionManager::addDepartment(const Department &department, Budget &budget)
    {
        if (budgets_.count(department))
            throw std::invalid_argument("department already has a budget: " + department.name());
        for (const auto &entry : budgets_)
            if (entry.second == &budget)
                throw std::invalid_argument("this budget already belongs to department " +
                                            entry.first.name());
        budgets_.emplace(department, &budget);
    }

    std::vector<Department> AcquisitionManager::departments() const
    {
        std::vector<Department> result;
        for (const auto &entry : budgets_)
            result.push_back(entry.first);
        return result;
    }

    Budget *AcquisitionManager::findBudget(const Department &department) const
    {
        auto it = budgets_.find(department);
        return it == budgets_.end() ? nullptr : it->second;
    }

    Budget &AcquisitionManager::lookupBudget(const Department &department) const
    {
        if (Budget *budget = findBudget(department))
            return *budget;
        throw UnknownDepartmentError(department.name());
    }

    const Budget &AcquisitionManager::budgetFor(const Department &department) const
    {
        return lookupBudget(department);
    }

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
        return canPurchase(id, quantity, Department::general(), reason);
    }

    bool AcquisitionManager::canPurchase(const std::string &id, int quantity,
                                         const Department &department,
                                         std::string *reason) const
    {
        std::string why;
        if (const Resource *r = catalog_.find(id))
        {
            if (const Budget *budget = findBudget(department))
            {
                if (quantity <= 0)
                    why = "quantity must be positive";
                else
                    why = budget->check(r->category(), quantity, pricing(*r, quantity).total(), id);
            }
            else
            {
                why = "unknown department: " + department.name();
            }
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
                                               std::string reason,
                                               const Department &department)
    {
        history_.push_back(PurchaseRecord{
            nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
            r ? r->category() : ResourceCategory::Book, qty, price.total(), approved,
            std::move(reason), price.preTax, price.tax, false, 0, department});
        return history_.back();
    }

    const PurchaseRecord &AcquisitionManager::purchase(const std::string &id, int quantity)
    {
        return purchase(id, quantity, Department::general());
    }

    const PurchaseRecord &AcquisitionManager::purchase(const std::string &id, int quantity,
                                                       const Department &department)
    {
        const Resource &r = catalog_.get(id);       // may throw NotFoundError
        Budget &budget = lookupBudget(department);  // may throw UnknownDepartmentError
        const Pricing price = pricing(r, quantity); // may throw invalid_argument
        // Quotas and the budget are checked against the post-tax cost.
        budget.commit(r.category(), quantity, price.total(), id); // may throw quota/budget errors
        catalog_.addHoldings(id, quantity);
        return record(&r, id, quantity, price, true, {}, department);
    }

    const PurchaseRecord &AcquisitionManager::cancel(int orderNo)
    {
        const PurchaseRecord *found = nullptr;
        for (const auto &rec : history_)
            if (rec.orderNo == orderNo)
                found = &rec;

        const std::string label = "order " + std::to_string(orderNo);
        if (!found)
            throw std::invalid_argument("no such order: " + std::to_string(orderNo));
        if (found->cancellation)
            throw std::invalid_argument(label + " is a cancellation and cannot be cancelled");
        if (!found->approved)
            throw std::invalid_argument(label + " was rejected and cannot be cancelled");
        if (isCancelled(orderNo))
            throw std::invalid_argument(label + " is already cancelled");

        // Copy it: adding the cancellation record below may move the history.
        const PurchaseRecord original = *found;

        // Do the check that can fail first, so a failed cancel changes nothing.
        if (catalog_.holdings(original.resourceId) < original.quantity) // may throw NotFoundError
            throw std::invalid_argument("not enough holdings of " + original.resourceId +
                                        " to cancel " + label);

        // Refund exactly what was charged (stored post-tax cost), not a new price:
        // prices and tax rates may have changed since the purchase.
        // The refund goes to the department's budget the order was charged to.
        lookupBudget(original.department)
            .refund(original.category, original.quantity, original.cost, original.resourceId);
        catalog_.addHoldings(original.resourceId, -original.quantity);

        history_.push_back(PurchaseRecord{
            nextOrderNo_++, original.resourceId, original.title, original.category, original.quantity, original.cost * -1, false, {}, original.preTaxCost * -1, original.tax * -1, true, original.orderNo, original.department});
        cancelledOrders_.insert(orderNo);
        return history_.back();
    }

    std::vector<PurchaseRecord> AcquisitionManager::processBatch(
        const std::vector<PurchaseRequest> &reqs)
    {
        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());
        for (const auto &req : reqs)
        {
            const Department department =
                req.department ? *req.department : Department::general();
            const Resource *r = catalog_.find(req.resourceId);
            const Budget *budget = findBudget(department);
            Pricing price{};
            std::string why;
            if (!r)
            {
                why = "resource not found: " + req.resourceId;
            }
            else if (!budget)
            {
                why = "unknown department: " + department.name();
            }
            else if (req.quantity <= 0)
            {
                why = "quantity must be positive";
            }
            else
            {
                price = pricing(*r, req.quantity);
                why = budget->check(r->category(), req.quantity, price.total(), req.resourceId);
            }

            if (why.empty())
            {
                results.push_back(purchase(req.resourceId, req.quantity, department));
            }
            else
            {
                results.push_back(
                    record(r, req.resourceId, req.quantity, price, false, why, department));
            }
        }
        return results;
    }

    Money AcquisitionManager::totalSpent() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (affectsTotals(rec))
                sum += rec.cost;
        return sum;
    }

    Money AcquisitionManager::totalSpentPreTax() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (affectsTotals(rec))
                sum += rec.preTaxCost;
        return sum;
    }

    Money AcquisitionManager::totalTax() const
    {
        Money sum;
        for (const auto &rec : history_)
            if (affectsTotals(rec))
                sum += rec.tax;
        return sum;
    }

    void AcquisitionManager::printReport(std::ostream &os) const
    {
        os << "Order history (" << history_.size() << " orders)\n";
        for (const auto &rec : history_)
        {
            const char *status =
                rec.cancellation ? "CANCELLED" : (rec.approved ? "APPROVED" : "REJECTED");
            os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
               << std::setw(10) << status << std::setw(6)
               << rec.resourceId << " x" << std::setw(3) << rec.quantity << " "
               << std::setw(12) << std::right << rec.cost.toString() << std::left << "  "
               << rec.title;
            if (rec.cancellation)
                os << "  (cancels order #" << rec.cancelsOrderNo << ")";
            if (!rec.approved && !rec.cancellation)
                os << "\n        reason: " << rec.reason;
            os << "\n";
        }
        os << "Total pre-tax: " << totalSpentPreTax() << "\n";
        os << "Total tax: " << totalTax() << "\n";
        os << "Total spent (post-tax): " << totalSpent() << "\n";
    }

} // namespace bookmgmt