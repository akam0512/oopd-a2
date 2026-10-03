# Assignment 2 – Answers

## Attempted questions

Q1

## Q1 – Journal

-I created a Journal class that inherits from Resource. It implements its own category(), costFor(), and printDetails() functions.
-The cost of a journal depends on the number of copies and the subscription period. So, costFor(copies) calculates the cost as unit price × number of copies × subscription years.
-The subscription period cannot be less than 1 year. If an invalid value is given, the constructor throws std::invalid_argument.
-I also added Journal to ResourceCategory, categoryName(), and kAllCategories so that journals can be included when setting quotas and displaying budget information.

## Q2 – EBook

## Q2 – EBook

- EBook derives from ElectronicResource, so seat-based pricing (platform fee + price × seats) and isDigital() are inherited unchanged.
- Added a new category value EBook (also in categoryName() and kAllCategories) so e-books can have a quota separate from other electronic resources.
- printDetails() calls ElectronicResource::printDetails() first, then prints authors, ISBN, file format and DRM status.
- Duplication answer: Book and EBook both store authors and an ISBN, so the two fields, their accessors, the matching constructor parameters and the code in printDetails() that prints them (using joinAuthors) are written twice. Inheritance alone cannot remove this. EBook has to derive from ElectronicResource to get seat-based pricing, so it cannot also derive from Book: that would give it two Resource base subobjects (the diamond problem), which only virtual inheritance could fix, and that adds a lot of complexity. A better fix is composition: put authors and ISBN in a small class (for example BibliographicInfo, with a print helper) and let both Book and EBook hold one as a member. Inheriting from that small class as a mixin would also work, but composition is simpler.

## Q3 – AudioBook and Thesis

- AudioBook: I chose ElectronicResource as the base class because an audiobook is a digital resource that can be accessed by users. So it can use the existing seat-based pricing and other features of ElectronicResource. If I derived it directly from Resource, I would have to implement that logic again. Book is also not a good choice because it contains print-book details such as edition and binding. An audiobook only needs to add its narrator and duration, with duration being positive.

- Thesis: I chose Resource directly because a thesis does not really fit the Book class. It does not need things like ISBN, edition, or binding, and it is usually free. Resource already allows the price to be zero, so the normal costFor() can handle it without extra pricing logic. The thesis adds the university, degree, and supervisor information and has its own category.
