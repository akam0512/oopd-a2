// MT26014-Akanksha Mishra
#include "bookmgmt/Resource.h"

#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    const char *categoryName(ResourceCategory c)
    {
        switch (c)
        {
        case ResourceCategory::Book:
            return "Book";
        case ResourceCategory::ElectronicResource:
            return "ElectronicResource";
        case ResourceCategory::Journal:
            return "Journal";
        case ResourceCategory::EBook:
            return "EBook";
        case ResourceCategory::AudioBook:
            return "AudioBook";
        case ResourceCategory::Thesis:
            return "Thesis";
        }

        return "Unknown";
    }

    Resource::Resource(std::string id, std::string title, std::string publisher,
                       int year, Money unitPrice)
        : id_(std::move(id)),
          title_(std::move(title)),
          publisher_(std::move(publisher)),
          year_(year),
          unitPrice_(unitPrice)
    {
        if (id_.empty())
            throw std::invalid_argument("resource id must not be empty");
        if (title_.empty())
            throw std::invalid_argument("resource title must not be empty");
        if (unitPrice_.isNegative())
            throw std::invalid_argument("price must not be negative");
    }

    void Resource::setUnitPrice(Money price)
    {
        if (price.isNegative())
            throw std::invalid_argument("price must not be negative");
        unitPrice_ = price;
    }
    void Resource::setOffer(const std::string &vendor, Money price)
    {
        if (vendor.find_first_not_of(' ') == std::string::npos)
            throw std::invalid_argument("vendor name must not be empty");
        if (price.isNegative())
            throw std::invalid_argument("vendor price must not be negative");
        for (VendorOffer &offer : offers_)
        {
            if (offer.vendor == vendor)
            {
                offer.price = price; // already listed: change its price
                return;
            }
        }
        offers_.push_back({vendor, price});
    }

    const VendorOffer *Resource::cheapestOffer() const
    {
        const VendorOffer *best = nullptr;
        for (const VendorOffer &offer : offers_)
            if (!best || offer.price < best->price)
                best = &offer;
        return best;
    }

    Money Resource::purchasePrice() const
    {
        const VendorOffer *best = cheapestOffer();
        return best ? best->price : unitPrice_;
    }
    void Resource::requirePositive(int quantity)
    {
        if (quantity <= 0)
            throw std::invalid_argument("quantity must be positive");
    }

    // Q5 helper (new): print items get kBulkDiscountPercent% off an order of
    // kBulkMinCopies or more copies. Used by the costFor overrides in Book,
    // Journal and Thesis.
    Money Resource::applyPrintBulkDiscount(Money total, int copies)
    {
        if (copies < kBulkMinCopies)
            return total;
        return total.scaledByPercent(100 - kBulkDiscountPercent);
    }

    Money Resource::costFor(int quantity) const
    {
        requirePositive(quantity);
        return purchasePrice() * quantity;
    }

    void Resource::print(std::ostream &os) const
    {
        os << categoryName(category()) << " " << id_ << "\n"
           << "  title: " << title_ << "\n"
           << "  publisher: " << publisher_ << "\n"
           << "  year: " << year_ << "\n"
           << "  unit price: " << unitPrice_ << "\n";
        for (const VendorOffer &offer : offers_)
            os << "  vendor: " << offer.vendor << " @ " << offer.price << "\n";
        printDetails(os);
    }

    void Resource::printDetails(std::ostream &) const {}

    std::string Resource::summary() const
    {
        std::ostringstream os;
        os << "[" << categoryName(category()) << "] " << id_ << "  " << title_
           << " (" << year_ << ")  @ " << unitPrice_;
        return os.str();
    }

    std::ostream &operator<<(std::ostream &os, const Resource &r)
    {
        r.print(os);
        return os;
    }

} // namespace bookmgmt