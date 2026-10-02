#pragma once
// Exception hierarchy. Catch LibraryError to handle any library-specific failure.

#include <stdexcept>
#include <string>

namespace bookmgmt {

class LibraryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NotFoundError : public LibraryError {
public:
    explicit NotFoundError(const std::string& id)
        : LibraryError("resource not found: " + id) {}
};

class DuplicateIdError : public LibraryError {
public:
    explicit DuplicateIdError(const std::string& id)
        : LibraryError("duplicate resource id: " + id) {}
};

// Thrown when a purchase would exceed a per-category quota (units or spend).
class QuotaExceededError : public LibraryError {
public:
    using LibraryError::LibraryError;
};

// Thrown when a purchase would exceed the overall budget.
class BudgetExceededError : public LibraryError {
public:
    using LibraryError::LibraryError;
};

}  // namespace bookmgmt
