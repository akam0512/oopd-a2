#pragma once
// Exception hierarchy. Catch LibraryError to handle any library-specific failure.

#include <stdexcept>
#include <string>

namespace bookmgmt
{

    class LibraryError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class NotFoundError : public LibraryError
    {
    public:
        explicit NotFoundError(const std::string &id)
            : LibraryError("resource not found: " + id) {}
    };

    class DuplicateIdError : public LibraryError
    {
    public:
        explicit DuplicateIdError(const std::string &id)
            : LibraryError("duplicate resource id: " + id) {}
    };

    // Thrown when a purchase would exceed a per-category quota (units or spend).
    class QuotaExceededError : public LibraryError
    {
    public:
        using LibraryError::LibraryError;
    };

    // Thrown when a purchase would exceed the overall budget.
    class BudgetExceededError : public LibraryError
    {
    public:
        using LibraryError::LibraryError;
    };

    // Q9
    class UnknownDepartmentError : public LibraryError
    {
    public:
        explicit UnknownDepartmentError(const std::string &name)
            : LibraryError("unknown department: " + name) {}
    };
    // Q14: thrown when every copy (or seat) of a resource is already in use.
    class NotAvailableError : public LibraryError
    {
    public:
        using LibraryError::LibraryError;
    };

    // Q15: thrown when amounts in different currencies are added, subtracted or compared.
    class CurrencyMismatchError : public LibraryError
    {
    public:
        CurrencyMismatchError(const std::string &a, const std::string &b)
            : LibraryError("cannot mix currencies: " + a + " and " + b) {}
    };

} // namespace bookmgmt
