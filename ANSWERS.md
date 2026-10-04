# Assignment 2 – Answers

## Attempted questions

Q1, Q2, Q3, Q4, Q5, Q6, Q7

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
