#pragma once
// Resource: abstract base class for every item the library can hold or buy.
//
//   Resource (abstract)
//   ├── Book
//   └── ElectronicResource

#include <iosfwd>
#include <string>
#include <vector>
#include "bookmgmt/Money.h"

namespace bookmgmt
{

    // EXTENSION POINT: when adding a new resource type, add a category here,
    // a matching name in categoryName() (Resource.cpp) and an entry in
    // kAllCategories (Budget.cpp).
    enum class ResourceCategory
    {
        Book,
        ElectronicResource,
        Journal,
        EBook,
        AudioBook,
        Thesis
    };

    const char *categoryName(ResourceCategory c);
    // Q12: one vendor's price for a title.
    struct VendorOffer
    {
        std::string vendor;
        Money price;
    };
    class Resource
    {
    public:
        Resource(std::string id, std::string title, std::string publisher,
                 int year, Money unitPrice);
        virtual ~Resource() = default;

        // Resources are owned polymorphically (std::unique_ptr); no slicing copies.
        Resource(const Resource &) = delete;
        Resource &operator=(const Resource &) = delete;

        const std::string &id() const { return id_; }
        const std::string &title() const { return title_; }
        const std::string &publisher() const { return publisher_; }
        int year() const { return year_; }
        Money unitPrice() const { return unitPrice_; }
        void setUnitPrice(Money price);

        virtual ResourceCategory category() const = 0;
        virtual bool isDigital() const { return false; }

        // Q13: used by the catalog searches. A resource without authors returns
        // an empty list, and one without an ISBN/ISSN returns an empty string.
        virtual std::vector<std::string> authorNames() const { return {}; }
        virtual std::string standardNumber() const { return ""; } // ISBN or ISSN

        // Total cost of acquiring `quantity` units of this resource.
        // "Unit" means copies for print items and user seats for electronic
        // licences. Default: unitPrice * quantity. Throws if quantity <= 0.
        // EXTENSION POINT: override for discounts, fees, tiered pricing, ...
        virtual Money costFor(int quantity) const;

        // Writes a multi-line human-readable description.
        void print(std::ostream &os) const;
        // Single-line summary, e.g. "[Book] B001  Clean Code (2008)  @ 450.00".
        std::string summary() const;

        // Q12: the vendors that sell this title, with their prices. The list
        // price (unitPrice) is only used while there are no offers.
        // setOffer adds a vendor, or changes its price if it is already listed.
        // Throws std::invalid_argument for an empty vendor name or a negative price.
        void setOffer(const std::string &vendor, Money price);
        const std::vector<VendorOffer> &offers() const { return offers_; }
        // The cheapest offer (the first one added wins a tie), or nullptr if none.
        const VendorOffer *cheapestOffer() const;
        // The price per unit that purchases use: the cheapest vendor's price,
        // or the list price if there are no offers.
        Money purchasePrice() const;

    protected:
        // Subclasses append their own fields (one "  key: value" line each).
        virtual void printDetails(std::ostream &os) const;
        static void requirePositive(int quantity);
        // Q5: print items get kBulkDiscountPercent% off an order of kBulkMinCopies or more.
        static constexpr int kBulkMinCopies = 10;
        static constexpr int kBulkDiscountPercent = 10;
        static Money applyPrintBulkDiscount(Money total, int copies);

    private:
        std::string id_;
        std::string title_;
        std::string publisher_;
        int year_;
        Money unitPrice_;
        std::vector<VendorOffer> offers_;
    };

    std::ostream &operator<<(std::ostream &os, const Resource &r);

} // namespace bookmgmt
