#include "bookmgmt/Department.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt
{

    Department::Department(std::string name) : name_(std::move(name))
    {
        if (name_.find_first_not_of(' ') == std::string::npos)
            throw std::invalid_argument("department name must not be empty");
    }

    Department Department::general() { return Department("General"); }

    std::ostream &operator<<(std::ostream &os, const Department &d) { return os << d.name(); }

} // namespace bookmgmt