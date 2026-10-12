#pragma once
// Q9: a department (Computer Science, Physics, ...) that has its own budget.

#include <iosfwd>
#include <string>

namespace bookmgmt
{

    class Department
    {
    public:
        // Throws std::invalid_argument if the name is empty or only spaces.
        // Names are compared exactly: "Physics" and "physics" are different.
        explicit Department(std::string name);

        // The department used when a request does not name one.
        static Department general();

        const std::string &name() const { return name_; }

        friend bool operator==(const Department &a, const Department &b)
        {
            return a.name_ == b.name_;
        }
        friend bool operator!=(const Department &a, const Department &b) { return !(a == b); }
        // Ordering by name, so a Department can be a key in std::map / std::set.
        friend bool operator<(const Department &a, const Department &b)
        {
            return a.name_ < b.name_;
        }

    private:
        std::string name_;
    };

    std::ostream &operator<<(std::ostream &os, const Department &d);

} // namespace bookmgmt