#pragma once
// Resource: abstract base class for every item the library can hold or buy.
//
//   Resource (abstract)
//   ├── Book
//   └── ElectronicResource

#include <iosfwd>
#include <string>

#include "bookmgmt/Money.h"

namespace bookmgmt {

// EXTENSION POINT: when adding a new resource type, add a category here,
// a matching name in categoryName() (Resource.cpp) and an entry in
// kAllCategories (Budget.cpp).
enum class ResourceCategory { Book, ElectronicResource };

const char* categoryName(ResourceCategory c);

class Resource {
public:
    Resource(std::string id, std::string title, std::string publisher,
             int year, Money unitPrice);
    virtual ~Resource() = default;

    // Resources are owned polymorphically (std::unique_ptr); no slicing copies.
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& publisher() const { return publisher_; }
    int year() const { return year_; }
    Money unitPrice() const { return unitPrice_; }
    void setUnitPrice(Money price);

    virtual ResourceCategory category() const = 0;
    virtual bool isDigital() const { return false; }

    // Total cost of acquiring `quantity` units of this resource.
    // "Unit" means copies for print items and user seats for electronic
    // licences. Default: unitPrice * quantity. Throws if quantity <= 0.
    // EXTENSION POINT: override for discounts, fees, tiered pricing, ...
    virtual Money costFor(int quantity) const;

    // Writes a multi-line human-readable description.
    void print(std::ostream& os) const;
    // Single-line summary, e.g. "[Book] B001  Clean Code (2008)  @ 450.00".
    std::string summary() const;

protected:
    // Subclasses append their own fields (one "  key: value" line each).
    virtual void printDetails(std::ostream& os) const;
    static void requirePositive(int quantity);

private:
    std::string id_;
    std::string title_;
    std::string publisher_;
    int year_;
    Money unitPrice_;
};

std::ostream& operator<<(std::ostream& os, const Resource& r);

}  // namespace bookmgmt
