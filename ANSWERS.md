# Assignment 2 – Answers

## Attempted questions

Q1

## Q1 – Journal

-I created a Journal class that inherits from Resource. It implements its own category(), costFor(), and printDetails() functions.
-The cost of a journal depends on the number of copies and the subscription period. So, costFor(copies) calculates the cost as unit price × number of copies × subscription years.
-The subscription period cannot be less than 1 year. If an invalid value is given, the constructor throws std::invalid_argument.
-I also added Journal to ResourceCategory, categoryName(), and kAllCategories so that journals can be included when setting quotas and displaying budget information.

## Q2 – EBook

- Design note (Q2): An e-book is also a type of book, so Book and EBook have some common code, such as authors, ISBN, their getters, constructor parameters, and printing these details.
- We cannot simply use inheritance to remove this duplication because Book inherits from Resource, while EBook needs to inherit from ElectronicResource to keep the seat-based pricing.
- One way to avoid this duplication is to create a separate class containing the common information like authors and ISBN, and then use that class inside both Book and EBook.
- I would prefer composition because it is simpler and avoids the complications of multiple inheritance.
