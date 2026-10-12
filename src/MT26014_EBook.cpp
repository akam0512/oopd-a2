// MT26014-Akanksha Mishra
#include "bookmgmt/MT26014_EBook.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    const char *formatName(FileFormat f)
    {
        switch (f)
        {
        case FileFormat::PDF:
            return "PDF";

        case FileFormat::EPUB:
            return "EPUB";

        case FileFormat::HTML:
            return "HTML";
        }

        return "Unknown";
    }

    EBook::EBook(std::string id, std::string title,
                 std::vector<std::string> authors,
                 std::string isbn,
                 std::string publisher, int year,
                 Money pricePerSeat,
                 std::string accessUrl,
                 FileFormat format,
                 bool drmProtected,
                 LicenseModel license,
                 Money platformFee)
        : ElectronicResource(std::move(id),
                             std::move(title),
                             std::move(publisher),
                             year,
                             pricePerSeat,
                             std::move(accessUrl),
                             license,
                             platformFee),
          authors_(std::move(authors)),
          isbn_(std::move(isbn)),
          format_(format),
          drmProtected_(drmProtected)
    {
    }

    void EBook::printDetails(std::ostream &os) const
    {
        ElectronicResource::printDetails(os);

        os << "  authors: ";

        for (std::size_t i = 0; i < authors_.size(); ++i)
        {
            if (i > 0)
            {
                os << ", ";
            }

            os << authors_[i];
        }

        os << "\n"
           << "  ISBN: " << isbn_ << "\n"
           << "  format: " << formatName(format_) << "\n"
           << "  DRM protected: "
           << (drmProtected_ ? "yes" : "no") << "\n";
    }

} // namespace bookmgmt