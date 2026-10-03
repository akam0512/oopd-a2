# Assignment 2 – Answers

## Attempted questions

Q1

## Q1 – Journal

-I created a Journal class that inherits from Resource. It implements its own category(), costFor(), and printDetails() functions.
-The cost of a journal depends on the number of copies and the subscription period. So, costFor(copies) calculates the cost as unit price × number of copies × subscription years.
-The subscription period cannot be less than 1 year. If an invalid value is given, the constructor throws std::invalid_argument.
-I also added Journal to ResourceCategory, categoryName(), and kAllCategories so that journals can be included when setting quotas and displaying budget information.
