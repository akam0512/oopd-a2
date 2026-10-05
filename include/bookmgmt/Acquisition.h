#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.
//
// Q9: every department has its own Budget. A request names the department it
// is charged to; if it names none, it is charged to Department::general().

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"
#include "bookmgmt/Department.h"
#include "bookmgmt/Tax.h"

namespace bookmgmt
{

    struct PurchaseRequest
    {
        std::string resourceId;
        int quantity;                                        // copies for print, seats for electronic
        std::optional<Department> department = std::nullopt; // Q9: empty = General
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
        std::string reason;                            // why it was rejected; empty if approved
        Money preTaxCost;                              // Q6: cost before tax
        Money tax;                                     // Q6: tax on that cost
        bool cancellation = false;                     // Q8: true for the record that cancels an order
        int cancelsOrderNo = 0;                        // Q8: the order a cancellation record cancels
        Department department = Department::general(); // Q9: the department charged
    };

    // Q9:
    struct DepartmentTotals
    {
        Money preTax;
        Money tax;
        Money postTax; // preTax + tax
    };

    class AcquisitionManager
    {
    public:
        // One budget, charged for every request: it is registered as the
        // "General" department. (The budget is owned by the caller.)
        AcquisitionManager(Catalog &catalog, Budget &budget, TaxPolicy taxPolicy = TaxPolicy{});
        // No budgets yet: register each department with addDepartment().
        explicit AcquisitionManager(Catalog &catalog, TaxPolicy taxPolicy = TaxPolicy{});

        // Q9: registers a department's budget (owned by the caller). Throws
        // std::invalid_argument if the department already has a budget, or if this
        // Budget object already belongs to another department.
        void addDepartment(const Department &department, Budget &budget);
        std::vector<Department> departments() const;
        // Throws UnknownDepartmentError.
        const Budget &budgetFor(const Department &department) const;
        // Q10: next year's budget for every department (see Budget::rollover; each
        // department's new allocation is its own total this year). The new budgets
        // are returned to the caller, who registers them with a new manager.
        std::map<Department, Budget> rolloverBudgets(int carryForwardPercent) const;

        const TaxPolicy &taxPolicy() const { return taxPolicy_; }
        void setTaxPolicy(TaxPolicy policy) { taxPolicy_ = policy; }

        // Price of a request without buying anything. Throws NotFoundError.
        // This is the pre-tax price; quoteWithTax() adds tax.
        Money quote(const std::string &id, int quantity) const;
        Money quoteWithTax(const std::string &id, int quantity) const;

        // True if the purchase would be approved; if not, `reason` explains why.
        // Without a department the purchase is charged to General.
        bool canPurchase(const std::string &id, int quantity,
                         std::string *reason = nullptr) const;
        bool canPurchase(const std::string &id, int quantity, const Department &department,
                         std::string *reason = nullptr) const;

        // Buys immediately. Throws NotFoundError, UnknownDepartmentError,
        // QuotaExceededError, BudgetExceededError or std::invalid_argument. On
        // success the department's budget and the holdings are updated and the
        // record is added to history. Without a department it is charged to General.
        const PurchaseRecord &purchase(const std::string &id, int quantity);
        const PurchaseRecord &purchase(const std::string &id, int quantity,
                                       const Department &department);

        // Q8: cancels an approved order. Refunds the budget and the quota usage
        // of the department it was charged to (using the amounts stored in the
        // original record), reduces the holdings, and adds a cancellation record
        // with negative amounts. The original record stays in the history.
        // Returns the cancellation record.
        // Throws std::invalid_argument, changing nothing, if the order does not
        // exist, was rejected, is itself a cancellation, or is already cancelled.
        const PurchaseRecord &cancel(int orderNo);
        bool isCancelled(int orderNo) const { return cancelledOrders_.count(orderNo) > 0; }

        // Processes requests in order; each is approved or rejected on its own
        // (never throws for a rejected request). Every outcome is recorded.
        // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
        std::vector<PurchaseRecord> processBatch(const std::vector<PurchaseRequest> &reqs);

        const std::vector<PurchaseRecord> &history() const { return history_; }
        // Totals over approved orders, minus cancelled ones (cancellation records
        // hold negative amounts). totalSpent() is post-tax (what the budgets were
        // charged, net of refunds) and equals totalSpentPreTax() + totalTax().
        Money totalSpent() const;
        Money totalSpentPreTax() const;
        Money totalTax() const;
        // Q9
        DepartmentTotals totalsFor(const Department &department) const;

        void printReport(std::ostream &os) const;
        // Q9: every department's budget, one after the other.
        void printBudgets(std::ostream &os) const;

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

        // The department's budget, or nullptr / UnknownDepartmentError if none.
        Budget *findBudget(const Department &department) const;
        Budget &lookupBudget(const Department &department) const;

        PurchaseRecord &record(const Resource *r, const std::string &id, int qty,
                               Pricing price, bool approved, std::string reason,
                               const Department &department);

        Catalog &catalog_;
        std::map<Department, Budget *> budgets_; // not owned
        TaxPolicy taxPolicy_;
        std::vector<PurchaseRecord> history_;
        std::set<int> cancelledOrders_; // Q8: numbers of orders that were cancelled
        int nextOrderNo_ = 1;
    };

} // namespace bookmgmt