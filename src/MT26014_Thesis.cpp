#include "bookmgmt/MT26014_Thesis.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    const char *degreeName(DegreeLevel d)
    {
        switch (d)
        {
        case DegreeLevel::Bachelor:
            return "Bachelor";
        case DegreeLevel::Master:
            return "Master";
        case DegreeLevel::PhD:
            return "PhD";
        }
        return "Unknown";
    }

    Thesis::Thesis(std::string id, std::string title, std::string university,
                   DegreeLevel degree, std::string supervisor, int year,
                   Money unitPrice)
        : Resource(std::move(id), std::move(title), std::move(university), year, unitPrice),
          degree_(degree),
          supervisor_(std::move(supervisor))
    {
        if (supervisor_.empty())
            throw std::invalid_argument("thesis supervisor must not be empty");
    }

    void Thesis::printDetails(std::ostream &os) const
    {
        // The university is already shown as the publisher by Resource::print.
        os << "  degree: " << degreeName(degree_) << "\n"
           << "  supervisor: " << supervisor_ << "\n";
    }
    // Q5: a thesis is a print item, so it gets the print bulk discount like Book and Journal.
    Money Thesis::costFor(int copies) const
    {
        requirePositive(copies);
        return applyPrintBulkDiscount(purchasePrice() * copies, copies);
    }

} // namespace bookmgmt