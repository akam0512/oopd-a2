#pragma once
// MT26014-Akanksha Mishra
#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt
{

    enum class DegreeLevel
    {
        Bachelor,
        Master,
        PhD
    };

    class Thesis : public Resource
    {
    public:
        // Base class choice (Q3): Thesis derives directly from Resource.
        // A thesis is a print item held by the library, usually given free by the
        // university, so it has no seat licences or platform fee (which rules out
        // ElectronicResource). It is also not a Book: a thesis has no ISBN, edition or
        // binding choice, and its own data (university, degree, supervisor) would not
        // fit Book's fields. Resource already allows a price of zero (only negative
        // prices are rejected), so the unit price defaults to Money{}, which makes a
        // thesis free by default. costFor() is overridden only to apply the print bulk
        // discount (Q5), like Book and Journal. The university is stored as the
        // Resource publisher, so it is printed by Resource and not duplicated.
        Thesis(std::string id, std::string title, std::string university,
               DegreeLevel degree, std::string supervisor, int year,
               Money unitPrice = Money{});

        const std::string &university() const { return publisher(); }
        DegreeLevel degree() const { return degree_; }
        const std::string &supervisor() const { return supervisor_; }

        ResourceCategory category() const override { return ResourceCategory::Thesis; }
        Money costFor(int copies) const override;

    protected:
        void printDetails(std::ostream &os) const override;

    private:
        DegreeLevel degree_;
        std::string supervisor_;
    };

    const char *degreeName(DegreeLevel d);

} // namespace bookmgmt