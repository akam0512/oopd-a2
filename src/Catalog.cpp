#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt
{

    namespace
    {
        std::string lower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char ch)
                           { return static_cast<char>(std::tolower(ch)); });
            return s;
        }
    } // namespace

    // Lower-case, without hyphens and spaces: "978-0-13 235088-4" -> "9780132350884".
    std::string normalizedNumber(const std::string &s)
    {
        std::string out;
        for (unsigned char ch : s)
            if (ch != '-' && ch != ' ')
                out += static_cast<char>(std::tolower(ch));
        return out;
    }

    Resource &Catalog::add(std::unique_ptr<Resource> r)
    {
        if (!r)
            throw std::invalid_argument("cannot add a null resource");
        const std::string id = r->id();
        if (items_.count(id))
            throw DuplicateIdError(id);
        Entry &e = items_[id];
        e.resource = std::move(r);
        return *e.resource;
    }

    bool Catalog::contains(const std::string &id) const { return items_.count(id) > 0; }

    Resource *Catalog::find(const std::string &id)
    {
        auto it = items_.find(id);
        return it == items_.end() ? nullptr : it->second.resource.get();
    }

    const Resource *Catalog::find(const std::string &id) const
    {
        auto it = items_.find(id);
        return it == items_.end() ? nullptr : it->second.resource.get();
    }

    Resource &Catalog::get(const std::string &id)
    {
        if (Resource *r = find(id))
            return *r;
        throw NotFoundError(id);
    }

    const Resource &Catalog::get(const std::string &id) const
    {
        if (const Resource *r = find(id))
            return *r;
        throw NotFoundError(id);
    }

    void Catalog::remove(const std::string &id)
    {
        if (items_.erase(id) == 0)
            throw NotFoundError(id);
    }

    int Catalog::holdings(const std::string &id) const
    {
        auto it = items_.find(id);
        if (it == items_.end())
            throw NotFoundError(id);
        return it->second.holdings;
    }

    void Catalog::addHoldings(const std::string &id, int units)
    {
        auto it = items_.find(id);
        if (it == items_.end())
            throw NotFoundError(id);
        if (it->second.holdings + units < 0)
            throw std::invalid_argument("holdings cannot become negative");
        it->second.holdings += units;
    }

    std::vector<const Resource *> Catalog::where(
        const std::function<bool(const Resource &)> &pred) const
    {
        std::vector<const Resource *> out;
        for (const auto &[id, entry] : items_)
            if (pred(*entry.resource))
                out.push_back(entry.resource.get());
        return out;
    }

    std::vector<const Resource *> Catalog::all() const
    {
        return where([](const Resource &)
                     { return true; });
    }

    std::vector<const Resource *> Catalog::byCategory(ResourceCategory c) const
    {
        return where([c](const Resource &r)
                     { return r.category() == c; });
    }

    std::vector<const Resource *> Catalog::searchTitle(const std::string &text) const
    {
        const std::string needle = lower(text);
        return where([&needle](const Resource &r)
                     { return lower(r.title()).find(needle) != std::string::npos; });
    }
    std::vector<const Resource *> Catalog::searchAuthor(const std::string &text) const
    {
        if (text.empty())
            throw std::invalid_argument("author search text must not be empty");
        const std::string needle = lower(text);
        return where([&needle](const Resource &r)
                     {
        for (const std::string& author : r.authorNames())
            if (lower(author).find(needle) != std::string::npos) return true;
        return false; });
    }

    std::vector<const Resource *> Catalog::searchIsbnIssn(const std::string &number) const
    {
        const std::string wanted = normalizedNumber(number);
        if (wanted.empty())
            throw std::invalid_argument("ISBN/ISSN must not be empty");
        return where([&wanted](const Resource &r)
                     { return normalizedNumber(r.standardNumber()) == wanted; });
    }

    std::vector<const Resource *> Catalog::searchYears(int from, int to) const
    {
        if (from > to)
            throw std::invalid_argument("the first year must not be after the last year");
        return where([from, to](const Resource &r)
                     { return r.year() >= from && r.year() <= to; });
    }

} // namespace bookmgmt
