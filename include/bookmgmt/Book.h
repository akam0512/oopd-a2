#pragma once

#include <string>
#include <vector>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

enum class Binding { Paperback, Hardcover };

class Book : public Resource {
public:
    Book(std::string id, std::string title, std::vector<std::string> authors,
         std::string isbn, std::string publisher, int year, Money unitPrice,
         int edition = 1, Binding binding = Binding::Paperback);

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    int edition() const { return edition_; }
    Binding binding() const { return binding_; }

    ResourceCategory category() const override { return ResourceCategory::Book; }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    int edition_;
    Binding binding_;
};

// Joins a list of author names: {"A", "B", "C"} -> "A, B and C"
std::string joinAuthors(const std::vector<std::string>& authors);

}  // namespace bookmgmt
