// Design note (Q2): An e-book is also a type of book, so Book and EBook
// have some common code, such as authors, ISBN, their getters,
// constructor parameters, and printing these details.
// We cannot simply use inheritance to remove this duplication because
// Book inherits from Resource, while EBook needs to inherit from
// ElectronicResource to keep the seat-based pricing.
// One way to avoid this duplication is to create a separate class
// containing the common information like authors and ISBN, and then
// use that class inside both Book and EBook.
// I would prefer composition because it is simpler and avoids the
// complications of multiple inheritance.
#pragma once

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt
{

    enum class FileFormat
    {
        PDF,
        EPUB,
        HTML
    };

    class EBook : public ElectronicResource
    {
    public:
        EBook(std::string id, std::string title, std::vector<std::string> authors,
              std::string isbn, std::string publisher, int year, Money pricePerSeat,
              std::string accessUrl, FileFormat format, bool drmProtected,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

        const std::vector<std::string> &authors() const { return authors_; }
        const std::string &isbn() const { return isbn_; }
        FileFormat format() const { return format_; }
        bool drmProtected() const { return drmProtected_; }

        ResourceCategory category() const override { return ResourceCategory::EBook; }
        std::vector<std::string> authorNames() const override { return authors_; }
        std::string standardNumber() const override { return isbn_; }

    protected:
        void printDetails(std::ostream &os) const override;

    private:
        std::vector<std::string> authors_;
        std::string isbn_;
        FileFormat format_;
        bool drmProtected_;
    };

    const char *formatName(FileFormat f);

} // namespace bookmgmt