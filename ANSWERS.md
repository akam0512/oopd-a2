# Assignment 2 – Answers

<----MT26014-Akanksha Mishra--->

## Attempted questions

Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13, Q14, Q15, Q16

## Q1 – Journal

- I created a Journal class that inherits from Resource. It implements its own category(), costFor(), and printDetails() functions.
- The cost of a journal depends on the number of copies and the subscription period. So, costFor(copies) calculates the cost as unit price × number of copies × subscription years.
- The subscription period cannot be less than 1 year. If an invalid value is given, the constructor throws std::invalid_argument.
- I also added Journal to ResourceCategory, categoryName(), and kAllCategories so that journals can be included when setting quotas and displaying budget information.

## Q2 – EBook

- EBook derives from ElectronicResource, so seat-based pricing (platform fee + price × seats) and isDigital() are inherited unchanged.
- Added a new category value EBook (also in categoryName() and kAllCategories) so e-books can have a quota separate from other electronic resources.
- printDetails() calls ElectronicResource::printDetails() first, then prints authors, ISBN, file format and DRM status.
- Duplication answer: Book and EBook both store authors and an ISBN, so the two fields, their accessors, the matching constructor parameters and the code in printDetails() that prints them (using joinAuthors) are written twice. Inheritance alone cannot remove this. EBook has to derive from ElectronicResource to get seat-based pricing, so it cannot also derive from Book: that would give it two Resource base subobjects (the diamond problem), which only virtual inheritance could fix, and that adds a lot of complexity. A better fix is composition: put authors and ISBN in a small class (for example BibliographicInfo, with a print helper) and let both Book and EBook hold one as a member. Inheriting from that small class as a mixin would also work, but composition is simpler.

## Q3 – AudioBook and Thesis

- AudioBook: I chose ElectronicResource as the base class because an audiobook is a digital resource that can be accessed by users. So it can use the existing seat-based pricing and other features of ElectronicResource. If I derived it directly from Resource, I would have to implement that logic again. Book is also not a good choice because it contains print-book details such as edition and binding. An audiobook only needs to add its narrator and duration, with duration being positive.

- Thesis: I chose Resource directly because a thesis does not really fit the Book class. It does not need things like ISBN, edition, or binding, and it is usually free. Resource already allows the price to be zero, so the price defaults to zero and a thesis needs no special pricing logic of its own. The thesis adds the university, degree, and supervisor information and has its own category. (In Q5, Thesis also overrides costFor() so that it gets the print bulk discount like Book and Journal.)

## Q4 – Hardcover surcharge

- I kept `unitPrice()` as the listed price. For hardcover books, I override `costFor()` and add the 20% surcharge there. This means quotes, purchases, quotas and the budget automatically use the correct cost.

- I used a named constant `kHardcoverSurchargePercent` instead of directly writing `20` in the calculation.

- The surcharge is applied to each copy, and then multiplied by the number of copies.

- Money is stored in whole paise, so I added `Money::scaledByPercent()` to calculate the 20% increase. It uses rounding half away from zero. This function can also be reused for later pricing questions.

- Paperback books remain unchanged. I changed the demo batch from `B002 x5` to `B002 x4` because five hardcover copies would exceed the Book spend quota.

## Q5 – Bulk discounts

- Book, Journal and Thesis get 10% discount when 10 or more copies are bought in one order. The quantity is checked for each order separately, not by adding quantities from previous orders.

- The discount is applied to the total cost after the hardcover price increase. I use `Money::scaledByPercent()` so the final amount is rounded only once. For Journals, only the number of copies is counted for the 10-copy limit, not the subscription years.

- I kept the original `Resource::costFor()` unchanged. I added `applyPrintBulkDiscount()` as a protected helper in `Resource` and use it in the `costFor()` overrides of Book, Journal and Thesis, so the same discount logic is not repeated.

- For electronic resources, the first 50 seats are charged at the full price and every seat after 50 costs half price. The platform fee is not discounted. This is handled in `ElectronicResource::costFor()`, so EBook and AudioBook get the same behavior automatically.

- Quotas and the overall budget use the discounted cost because they calculate the cost using `costFor()`.

## Q6 – Taxes

- I kept tax separate from the actual pricing. `costFor()` still gives the price before tax. I added a new `TaxPolicy` class to store the print and electronic tax rates. Both rates are 0% by default, and the setters only allow values from 0 to 100.

- I use `isDigital()` to decide which tax rate to apply. Electronic resources, EBooks and AudioBooks use the electronic tax rate, while Books, Journals and Theses use the print tax rate.

- Tax is calculated once for each order using the pre-tax cost. I use `Money::scaledByPercent()` so the tax is rounded half away from zero.

- Quotas and the overall budget are checked using the price after tax. This means an order that fits before tax can still be rejected if the post-tax amount exceeds a quota or the available budget.

- `PurchaseRecord::cost` stores the final amount charged after tax. I also added `preTaxCost` and `tax` so the report can show all three amounts. `totalSpent()` also uses the post-tax amount, so it stays consistent with the budget.

- The report now shows the total pre-tax amount, total tax and total post-tax amount. The existing `quote()` still gives the pre-tax price, while `quoteWithTax()` gives the price including tax.

- One limitation is that the tax rates are whole percentages, so values like 12.5% cannot be represented. Supporting such rates would require using a finer unit such as basis points.

## Q7 – Limit on different titles per category

- I added an optional `maxTitles` field to `Quota`. It defaults to "no limit" (`Quota::kNoLimit`), so every existing `{units, spend}` quota behaves exactly as before. A limit of 0 means no title can be bought in that category, and a negative limit is rejected by `setQuota` with `std::invalid_argument`.
- A "title" is a distinct resource id within a category. `Budget` keeps a map from title id to units bought for each category, so buying more copies of a title already bought does not increase the title count. Titles are counted per category: a Journal title does not count against the Book limit.
- If a purchase would add a new title when the category has already reached `maxTitles`, it is rejected with a `QuotaExceededError` (`Book title quota exceeded: ...`) and nothing changes.
- The checks now run in this order: input, unit quota, title quota, spend quota, overall budget. I put the title check right after the unit quota because both are count limits.
- `Budget::check()` and `Budget::commit()` take an optional `resourceId` argument. `AcquisitionManager` passes the id in `canPurchase`, `purchase` and `processBatch`. Callers that don't pass an id (the old signature) skip the title check, so existing code still works.
- I added `Budget::titlesRemaining()` and `Usage::titles()`, and the budget report has a new "Titles used/max" column.
- I stored units per title (a map) instead of a plain set so that Q8 (cancellation) can reduce a title's units and remove the title when it reaches zero.

## Q8 – Cancellation

- I use the order number to identify an order for cancellation. `cancel(orderNo)` only works for an approved order. If the order number is invalid, the order was rejected, it is already a cancellation record, or it was already cancelled, the function throws `std::invalid_argument` and does not change anything.

- I keep the original purchase record unchanged. Instead, I add a separate cancellation record with the same resource details and quantity, but with negative cost, pre-tax cost and tax. The cancellation record is marked separately so it can be shown as `CANCELLED`.

- For the refund, I use the original amounts stored in the purchase record instead of calculating the price again. This is important because the price or tax rate could have changed after the original purchase.

- `Budget::refund()` reverses the changes made by `commit()`. It gives back the category units, category spending and overall budget amount. It checks everything before changing anything, so a failed refund does not partially change the budget.

- The holdings are also reduced by the cancelled quantity. The holdings are checked before the refund, so if the resource is not available, the cancellation fails without changing the budget.

- For the Q7 title limit, I track the units for each title. Cancelling one of multiple orders for the same title keeps that title counted. When all units of a title are cancelled, that title is removed from the tracking, so its slot becomes available again.

- The spending totals include the cancellation records as negative amounts. Rejected orders are not included, so the totals show the actual net spending after cancellations.

- A cancellation always cancels the complete order. It is not possible to cancel only part of an order.

## Q9 – Department budgets

- I created a separate `Department` type instead of using a normal string. This makes department names safer to use and avoids accidentally mixing them with other strings.

- `AcquisitionManager` keeps a separate `Budget` for each department. A department must be registered only once, and the same `Budget` cannot be shared between two departments.

- Since each department has its own `Budget`, it also has its own overall budget, category quotas, title limits and usage.

- A purchase request includes the department it belongs to. The purchase is checked and charged only to that department's budget.

- I kept the old `AcquisitionManager(catalog, budget)` constructor working by treating that budget as the `"General"` department. This keeps the earlier code working without changes.

- If an unknown department is used, the purchase is rejected with an appropriate error/reason. The same applies when checking a purchase.

- Each purchase record stores its department. This is important for cancellation because the money and quota usage must be returned to the same department's budget.

- Holdings remain shared in the catalog, so they represent the library's total holdings across all departments.

- The report can show the department for each purchase and can also show totals for each department. `printBudgets()` shows the budget of every department.

- Taxes continue to use the same tax policy for the whole `AcquisitionManager`; they are not separate for each department.

## Q10 – Year-end rollover

- I made `Budget::rollover()` for creating the budget for the next year. The new budget gets the new allocation plus the given percentage of the money left unused this year. The unused money is taken from `remaining()`, so refunded money is also included.

- I also added `carryForwardAmount()` to calculate only the amount that will be carried forward. The percentage must be between 0 and 100, and the new allocation cannot be negative.

- The quotas are copied to the new budget, but the usage starts from zero. So the unit, spend and title limits are available again for the new year. The current year's budget is not changed.

- If departments are being used, `rolloverBudgets()` rolls over each department's budget separately. It returns the new budgets so they can be given to a new `AcquisitionManager`.

- I did not store the carried-forward amount separately in the new budget because it was not required for this question.

## Q11 – All-or-nothing batches

- I added an optional allOrNothing flag to processBatch(). If it is not given, the old behaviour stays the same.

- In all-or-nothing mode, I first check the requests on copies of the budgets. This is important because one request can affect the next request. For example, two requests of 3 units each cannot both fit in a quota of 5 units.

- If all requests pass the check, I buy all of them normally. If even one request fails, nothing is actually bought. All requests are recorded as rejected, with the failed request showing its actual reason and the others showing that they were not bought because of the failed request.

- The dry run uses the same checks as a normal purchase, so quotas, title limits, taxes, department budgets and invalid resources or departments are handled in the same way.

- I used one small helper function, rejectionReason(), for checking why a request would fail. This keeps the dry run and normal purchase logic consistent.

- I used a simple bool for the all-or-nothing option instead of adding another type, to keep the implementation simple.

## Q12 – Vendors

- Each resource keeps a list of vendor offers (`VendorOffer`: vendor name and price). `setOffer(vendor, price)` adds a vendor or changes its price if it is already listed; an empty vendor name or a negative price throws `std::invalid_argument`. `cheapestOffer()` returns the cheapest offer (the first one added wins a tie).
- `purchasePrice()` is the price per unit that purchases use: the cheapest vendor's price, or the list price (`unitPrice()`) if there are no offers. Every `costFor` (default, Book, Journal, Thesis, ElectronicResource) now uses `purchasePrice()` instead of `unitPrice()`, so the hardcover surcharge, bulk discounts, seat pricing, taxes, quotas and quotes all use the vendor price automatically. EBook and AudioBook inherit it. `unitPrice()` stays the list price and is what the catalog summary shows, so existing code is unchanged.
- Choosing by unit price is enough, because every pricing rule grows with the unit price, so the cheapest unit price is also the cheapest total.
- `PurchaseRecord` has a `vendor` field. An approved order records the cheapest vendor at the time of purchase; rejected orders have no vendor; a cancellation record copies the vendor of the original order. The report shows `from <vendor>`. A vendor's price change later does not affect earlier orders, and a cancellation refunds the amount stored in the original order.
- Limitation: a vendor only has a unit price. A vendor-specific platform fee for electronic resources is not modelled.

## Q13 – Searches

- I added two virtual functions in Resource: authorNames() and standardNumber(). By default, they return an empty list and an empty string. Book and EBook return their authors and ISBN, while Journal returns its ISSN. This keeps the search logic inside Catalog without needing to check the type of every resource.
- searchAuthor(text) searches for resources where any author's name contains the given text, without considering uppercase or lowercase. Narrators and supervisors are not treated as authors.
- searchIsbnIssn(number) searches for an exact ISBN or ISSN. While comparing, hyphens and spaces are ignored and the comparison is case-insensitive, so values like an ISSN ending with X are handled correctly.
- searchYears(from, to) searches for resources published between the given years, including both from and to. If from is greater than to, it throws std::invalid_argument.
- Empty author text or an empty ISBN/ISSN search throws std::invalid_argument.
- The search results are returned in ID order, the same way as the existing searchTitle() function.

## Q14 – Lending

- I added a new class, LendingDesk, which uses the catalog holdings. For print items it keeps track of copies, and for electronic items it keeps track of licensed seats. It stores what is currently being used by keeping the patron and resource id. It does not change the catalog itself.

- For print items, I used borrow and giveBack, while for electronic items I used openSession and closeSession. If the wrong function is used for a resource, std::invalid_argument is thrown. Journals and theses are treated as print items, while e-books and audiobooks are electronic.

- The number of people who can borrow or open a session is limited by the available holdings. available(id) gives the holdings minus the copies or seats currently in use, and it never becomes negative. If nothing is available, NotAvailableError is thrown and the failed operation does not change anything. Invalid patron names, returning something the patron does not have, or using the wrong type of operation gives std::invalid_argument, while an unknown resource id gives NotFoundError.

- A patron is just stored as a string. There is no per-patron limit given in the assignment, so the same patron can have multiple copies of the same item. Each giveBack returns one copy.

- One limitation is that cancelling an order from Q8 does not check whether the copies or seats are currently being used. If something is cancelled while in use, available() does not go below 0, and the existing loan/session can still be returned or closed.

## Q15 – Currency code in Money

- I added a three-letter currency code to `Money`. The code is stored in upper case. The currency can be given as the last argument in `Money::of(...)` and `Money::fromMinor(...)`. If the code is not exactly three letters, it throws `std::invalid_argument`. The default currency is INR, so the old code and output still work as before. `toString()` and `<<` still show only the amount, while `toStringWithCurrency()` also shows the currency code.

- If I try to add, subtract, or compare two amounts with different currencies, it throws `CurrencyMismatchError`, which is a `LibraryError`. The error message shows both currencies. This also applies to `==` instead of returning false. If `+=` or `-=` fails, the original value is not changed. Multiplication and `scaledByPercent()` keep the same currency.

- I allowed a zero `Money` value to work with any currency. This is needed because the library starts many totals with `Money{}`. Without this, adding the first USD amount to a zero total would fail. For example, `Money{} + USD 5` gives `USD 5`. But once a total already has a currency, it cannot be mixed with a different currency.

- The budget, quotas and resource prices must all use the same currency. If there is a currency mismatch, `Budget::evaluate` converts it into a rejection reason. Because of this, `canPurchase` returns false and `processBatch` rejects the request with the currency mismatch message. A direct `purchase` throws `std::invalid_argument`. No budget or purchase state is changed when this happens.

- I removed `constexpr` from the `Money` functions because the currency is stored using `std::string`, which cannot be used in constant expressions in C++17. Nothing in the project was using these functions as constant expressions.

- There is no currency conversion or exchange-rate support. Also, reports still show only the amount without the currency code, so the existing report format stays unchanged.

## Q16 – Test macros replaced by assert

- The tests used two macros with global counters: `CHECK(cond)` and `CHECK_THROWS(expr, ExType)`. Every `CHECK(cond)` is now `assert(cond)`, with the same condition.
- `assert` cannot say "this expression must throw this type", so `CHECK_THROWS` became a call to one small function template, `throws<ExType>(fn)`, which returns true only if `fn` throws `ExType` (or a type derived from it); no exception or a different one gives false. The test line is `assert(throws<ExType>([&] { (void)(expr); }))`. It is an ordinary function, not a macro, so it is type-checked.
- What does not change: every check still tests exactly the same condition. What has to change: `assert` aborts at the first failure (printing the file, line and expression) instead of counting failures and carrying on, so `main` prints "All tests passed" instead of a count.
- `assert` does nothing when `NDEBUG` is defined (for example in a Release build), and many checks have side effects (such as `assert(acq.purchase(...).approved)`), so the test file does `#undef NDEBUG` before `#include <cassert>`. I checked that a build with `-DNDEBUG` still runs every check.

---

## AI Disclosure

1. AI Tool Used

I used ChatGPT as an AI-assisted learning and development tool while working on this Object-Oriented Programming and Design (OOPD) Assignment 2.

2. Purpose of Using AI

The primary purpose of using AI was to improve my understanding of the assignment requirements, clarify object-oriented programming concepts, identify errors, and obtain guidance during implementation. I used it as a supporting tool throughout the development process rather than relying on it solely to produce a final submission.

3. Areas Where AI Assistance Was Used

Understanding requirements and design: I used ChatGPT to clarify the requirements of individual questions and understand how different classes, inheritance relationships, composition, and responsibilities should fit into the existing project structure.

Implementation guidance: I sought explanations and coding guidance while working on features involving different resource types, including journals, e-books, audiobooks, and theses. I also used AI assistance to understand the implementation requirements for pricing rules, discounts, taxation, budget management, vendor offers, purchase cancellation, lending, currency-aware monetary values, and related functionality.

Debugging and error resolution: I used ChatGPT to help understand compiler errors, unexpected behaviour, and potential implementation issues. I reviewed the suggested fixes in the context of my existing code and used compilation and test execution to check whether the changes worked as expected.

Testing and verification: I used AI assistance to understand how to test different features and identify cases that should be checked. I ran the project's test suite and demo program to verify the behaviour of the implementation. I also checked the build process and warning output during verification.

Documentation and submission preparation: I used ChatGPT for guidance on preparing and reviewing README.md and ANSWERS.md, explaining design decisions, improving the clarity of written explanations, and preparing the AI disclosure. I also used guidance for Git version control, file naming, and packaging the project for submission.

4. Review and Verification

I reviewed the suggestions provided by ChatGPT and made changes to the project as part of my development process. I used the available compiler, test suite, and demo program to verify the project after making changes. The final project was checked for successful compilation and test execution. Passing these checks provides evidence of functional correctness for the tested cases, although it does not independently guarantee that every possible case or assignment requirement is satisfied.

**Prompts and supporting materials:** I uploaded a few assignment-related prompts and materials to ChatGPT to obtain help with understanding the requirements, clarifying implementation details, and resolving specific doubts during development. The responses were used as guidance while working on the project. I reviewed the relevant suggestions and verified the implementation through compilation, testing, and demo execution.
