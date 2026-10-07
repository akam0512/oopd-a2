#pragma once
// Q14: lending. Patrons borrow and return print copies, and open and close
// sessions on electronic resources. What can be lent is limited by what the
// catalog holds: copies for print items, licensed seats for electronic ones.

#include <string>
#include <vector>

#include "bookmgmt/Catalog.h"

namespace bookmgmt
{

    class LendingDesk
    {
    public:
        // Reads the holdings from the catalog (it is not changed). Use the same
        // catalog as the AcquisitionManager, so purchases add copies and seats.
        explicit LendingDesk(const Catalog &catalog);

        // Print items: a patron borrows one copy and later gives it back.
        // Throws NotFoundError (unknown id), NotAvailableError (no copy free) and
        // std::invalid_argument (electronic item, empty patron name, or nothing to
        // give back). A failed call changes nothing.
        void borrow(const std::string &patron, const std::string &id);
        void giveBack(const std::string &patron, const std::string &id);

        // Electronic items: a patron opens one session (using one seat) and later
        // closes it. The same exceptions apply, with the print/electronic rule reversed.
        void openSession(const std::string &patron, const std::string &id);
        void closeSession(const std::string &patron, const std::string &id);

        // Copies on loan, or sessions open. Throws NotFoundError for an unknown id.
        int inUse(const std::string &id) const;
        // Copies or seats still free: holdings minus inUse(), never below zero.
        int available(const std::string &id) const;
        // The ids a patron has borrowed or has sessions on, in the order they were taken.
        std::vector<std::string> loansOf(const std::string &patron) const;

    private:
        struct Loan
        {
            std::string patron;
            std::string resourceId;
        };

        void lend(const std::string &patron, const std::string &id, bool digital);
        void takeBack(const std::string &patron, const std::string &id, bool digital);
        // Throws std::invalid_argument if the resource is not of the expected kind.
        static void requireKind(const Resource &r, bool digital);

        const Catalog &catalog_;
        std::vector<Loan> loans_; // every copy on loan and every open session
    };

} // namespace bookmgmt