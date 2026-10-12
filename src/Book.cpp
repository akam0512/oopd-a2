// MT26014-Akanksha Mishra
#include "bookmgmt/Book.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    std::string joinAuthors(const std::vector<std::string> &authors)
    {
        std::string out;
        for (std::size_t i = 0; i < authors.size(); ++i)
        {
            if (i > 0)
                out += (i + 1 == authors.size()) ? " and " : ", ";
            out += authors[i];
        }
        return out;
    }

    Book::Book(std::string id, std::string title, std::vector<std::string> authors,
               std::string isbn, std::string publisher, int year, Money unitPrice,
               int edition, Binding binding)
        : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
          authors_(std::move(authors)),
          isbn_(std::move(isbn)),
          edition_(edition),
          binding_(binding)
    {
        if (edition_ < 1)
            throw std::invalid_argument("edition must be >= 1");
    }

    // Q4: hardcover copies cost kHardcoverSurchargePercent% more than the listed
    // unit price. The surcharge is applied per copy and then multiplied, so the
    // cost of n copies is always n times the cost of one. unitPrice() itself is
    // unchanged (it stays the listed price).
    Money Book::costFor(int copies) const
    {
        requirePositive(copies);
        Money perCopy = purchasePrice();
        if (binding_ == Binding::Hardcover)
            perCopy = perCopy.scaledByPercent(100 + kHardcoverSurchargePercent);
        return applyPrintBulkDiscount(perCopy * copies, copies);
    }

    void Book::printDetails(std::ostream &os) const
    {
        os << "  authors: " << joinAuthors(authors_) << "\n"
           << "  isbn: " << isbn_ << "\n"
           << "  edition: " << edition_ << "\n"
           << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
           << "\n";
    }

} // namespace bookmgmt