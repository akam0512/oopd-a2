// MT26014-Akanksha Mishra
#include "bookmgmt/MT26014_Lending.h"

#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    LendingDesk::LendingDesk(const Catalog &catalog) : catalog_(catalog) {}

    void LendingDesk::requireKind(const Resource &r, bool digital)
    {
        if (r.isDigital() == digital)
            return;
        throw std::invalid_argument(r.id() + (r.isDigital()
                                                  ? " is an electronic resource: use openSession/closeSession"
                                                  : " is a print item: use borrow/giveBack"));
    }

    void LendingDesk::lend(const std::string &patron, const std::string &id, bool digital)
    {
        if (patron.find_first_not_of(' ') == std::string::npos)
            throw std::invalid_argument("patron name must not be empty");
        const Resource &r = catalog_.get(id); // throws NotFoundError
        requireKind(r, digital);

        if (available(id) <= 0)
        {
            const std::string what = digital ? "seat" : "copy";
            throw NotAvailableError("no " + what + " of " + id + " is available: " +
                                    std::to_string(inUse(id)) + " of " +
                                    std::to_string(catalog_.holdings(id)) +
                                    (digital ? " in use" : " on loan"));
        }
        loans_.push_back({patron, id});
    }

    void LendingDesk::takeBack(const std::string &patron, const std::string &id, bool digital)
    {
        requireKind(catalog_.get(id), digital); // throws NotFoundError / invalid_argument
        for (auto it = loans_.begin(); it != loans_.end(); ++it)
        {
            if (it->patron == patron && it->resourceId == id)
            {
                loans_.erase(it);
                return;
            }
        }
        throw std::invalid_argument(patron + (digital ? " has no open session on " : " has not borrowed ") + id);
    }

    void LendingDesk::borrow(const std::string &patron, const std::string &id)
    {
        lend(patron, id, false);
    }

    void LendingDesk::giveBack(const std::string &patron, const std::string &id)
    {
        takeBack(patron, id, false);
    }

    void LendingDesk::openSession(const std::string &patron, const std::string &id)
    {
        lend(patron, id, true);
    }

    void LendingDesk::closeSession(const std::string &patron, const std::string &id)
    {
        takeBack(patron, id, true);
    }

    int LendingDesk::inUse(const std::string &id) const
    {
        catalog_.get(id); // throws NotFoundError for an unknown id
        int count = 0;
        for (const Loan &loan : loans_)
            if (loan.resourceId == id)
                ++count;
        return count;
    }

    int LendingDesk::available(const std::string &id) const
    {
        const int free = catalog_.holdings(id) - inUse(id);
        return free > 0 ? free : 0;
    }

    std::vector<std::string> LendingDesk::loansOf(const std::string &patron) const
    {
        std::vector<std::string> ids;
        for (const Loan &loan : loans_)
            if (loan.patron == patron)
                ids.push_back(loan.resourceId);
        return ids;
    }

} // namespace bookmgmt