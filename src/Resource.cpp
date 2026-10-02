#include "bookmgmt/Resource.h"

#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bookmgmt {

const char* categoryName(ResourceCategory c) {
    switch (c) {
        case ResourceCategory::Book: return "Book";
        case ResourceCategory::ElectronicResource: return "ElectronicResource";
    }
    return "Unknown";
}

Resource::Resource(std::string id, std::string title, std::string publisher,
                   int year, Money unitPrice)
    : id_(std::move(id)),
      title_(std::move(title)),
      publisher_(std::move(publisher)),
      year_(year),
      unitPrice_(unitPrice) {
    if (id_.empty()) throw std::invalid_argument("resource id must not be empty");
    if (title_.empty()) throw std::invalid_argument("resource title must not be empty");
    if (unitPrice_.isNegative()) throw std::invalid_argument("price must not be negative");
}

void Resource::setUnitPrice(Money price) {
    if (price.isNegative()) throw std::invalid_argument("price must not be negative");
    unitPrice_ = price;
}

void Resource::requirePositive(int quantity) {
    if (quantity <= 0) throw std::invalid_argument("quantity must be positive");
}

Money Resource::costFor(int quantity) const {
    requirePositive(quantity);
    return unitPrice_ * quantity;
}

void Resource::print(std::ostream& os) const {
    os << categoryName(category()) << " " << id_ << "\n"
       << "  title: " << title_ << "\n"
       << "  publisher: " << publisher_ << "\n"
       << "  year: " << year_ << "\n"
       << "  unit price: " << unitPrice_ << "\n";
    printDetails(os);
}

void Resource::printDetails(std::ostream&) const {}

std::string Resource::summary() const {
    std::ostringstream os;
    os << "[" << categoryName(category()) << "] " << id_ << "  " << title_
       << " (" << year_ << ")  @ " << unitPrice_;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Resource& r) {
    r.print(os);
    return os;
}

}  // namespace bookmgmt
