#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"
#include "bookmgmt/Tax.h"

namespace bookmgmt
{

    struct PurchaseRequest
    {
        std::string resourceId;
        int quantity; // copies for print, seats for electronic
    };

    struct PurchaseRecord
    {
        int orderNo;
        std::string resourceId;
        std::string title;
        ResourceCategory category;
        int quantity;
        Money cost; // amount charged to the budget: post-tax (preTaxCost + tax)
        bool approved;
        std::string reason;        // why it was rejected; empty if approved
        Money preTaxCost;          // Q6: cost before tax
        Money tax;                 // Q6: tax on that cost
        bool cancellation = false; // Q8: true for the record that cancels an order
        int cancelsOrderNo = 0;    // Q8: the order a cancellation record cancels
    };

    class AcquisitionManager
    {
    public:
        AcquisitionManager(Catalog &catalog, Budget &budget, TaxPolicy taxPolicy = TaxPolicy{});

        const TaxPolicy &taxPolicy() const { return taxPolicy_; }
        void setTaxPolicy(TaxPolicy policy) { taxPolicy_ = policy; }

        // Price of a request without buying anything. Throws NotFoundError.
        // This is the pre-tax price; quoteWithTax() adds tax.
        Money quote(const std::string &id, int quantity) const;
        Money quoteWithTax(const std::string &id, int quantity) const;

        // True if the purchase would be approved; if not, `reason` explains why.
        bool canPurchase(const std::string &id, int quantity,
                         std::string *reason = nullptr) const;

        // Buys immediately. Throws NotFoundError, QuotaExceededError,
        // BudgetExceededError or std::invalid_argument. On success the budget
        // and holdings are updated and the record is added to history.
        const PurchaseRecord &purchase(const std::string &id, int quantity);

        // Q8: cancels an approved order. Refunds the budget and the quota usage
        // (using the amounts stored in the original record), reduces the holdings,
        // and adds a cancellation record with negative amounts. The original
        // record stays in the history. Returns the cancellation record.
        // Throws std::invalid_argument, changing nothing, if the order does not
        // exist, was rejected, is itself a cancellation, or is already cancelled.
        const PurchaseRecord &cancel(int orderNo);
        bool isCancelled(int orderNo) const { return cancelledOrders_.count(orderNo) > 0; }

        // Processes requests in order; each is approved or rejected on its own
        // (never throws for a rejected request). Every outcome is recorded.
        // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
        std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest> &reqs);

        const std::vector<PurchaseRecord> &history() const { return history_; }
        // Totals over approved orders. totalSpent() is post-tax (what the budget
        // was charged) and equals totalSpentPreTax() + totalTax().
        Money totalSpent() const;
        Money totalSpentPreTax() const;
        Money totalTax() const;

        void printReport(std::ostream &os) const;

    private:
        // Cost of a request before and after tax. Quotas and the budget are
        // always checked against total() (post-tax).
        struct Pricing
        {
            Money preTax;
            Money tax;
            Money total() const { return preTax + tax; }
        };
        Pricing pricing(const Resource &r, int quantity) const;

        PurchaseRecord &record(const Resource *r, const std::string &id, int qty,
                               Pricing price, bool approved, std::string reason);

        Catalog &catalog_;
        Budget &budget_;
        TaxPolicy taxPolicy_;
        std::vector<PurchaseRecord> history_;
        std::set<int> cancelledOrders_; // Q8: numbers of orders that were cancelled
        int nextOrderNo_ = 1;
    };

} // namespace bookmgmt