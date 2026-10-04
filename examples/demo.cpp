// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main()
{
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    catalog.emplace<Journal>("J001", "Nature", "0028-0836", "Springer Nature", 2026,
                             Money::of(300), 51, 2);
    catalog.emplace<EBook>("E001", "Effective Modern C++",
                           std::vector<std::string>{"Scott Meyers"}, "978-1491903995",
                           "O'Reilly", 2014, Money::of(200), "https://ebooks.example/emc",
                           FileFormat::PDF, true, LicenseModel::AnnualSubscription,
                           Money::of(500));
    catalog.emplace<AudioBook>("AB01", "Project Hail Mary", "Ballantine", 2021,
                               Money::of(100), "https://audio.example/phm",
                               LicenseModel::AnnualSubscription, Money::of(300),
                               "Ray Porter", 480);
    catalog.emplace<Thesis>("T001", "Deep Learning for Hospital Records", "IIT Delhi",
                            DegreeLevel::PhD, "Dr. Rao", 2025);

    std::cout << "=== Catalog ===\n";
    for (const Resource *r : catalog.all())
        std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n"
              << catalog.get("R001");
    std::cout << "\n=== Details of J001 ===\n"
              << catalog.get("J001");
    std::cout << "\n=== Details of E001 ===\n"
              << catalog.get("E001");
    std::cout << "\n=== Details of AB01 ===\n"
              << catalog.get("AB01");
    std::cout << "\n=== Details of T001 ===\n"
              << catalog.get("T001");

    Budget budget(Money::of(25000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {3, Money::of(2000)});
    budget.setQuota(ResourceCategory::EBook, {10, Money::of(3000)});
    budget.setQuota(ResourceCategory::AudioBook, {4, Money::of(2000)});
    budget.setQuota(ResourceCategory::Thesis, {5, Money::of(0)});

    AcquisitionManager acq(catalog, budget);

    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";
    std::cout << "10 copies of B001 = " << acq.quote("B001", 10) << "  (10% bulk discount)\n";
    std::cout << "60 seats of R001  = " << acq.quote("R001", 60) << "  (seats after the 50th at half price)\n";

    acq.processBatch({
        {"B001", 4},  // 1800  ok
        {"B002", 4},  // 4 x 1440 = 5760  ok (hardcover: 1200 + 20%)  -> book spend 7560
        {"B001", 1},  // 450   rejected: book spend quota (440 left)
        {"R001", 20}, // 5000  ok
        {"R002", 25}, // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15}, // 6000  ok  -> e-resource spend 11000
        {"R002", 5},  // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},  // rejected: unknown id
        {"J001", 2},  // 1200  ok  -> journal spend 1200
        {"J001", 2},  // 1200  rejected: journal unit quota (1 unit left)
        {"E001", 5},  // 500 + 1000 = 1500  ok
        {"E001", 8},  // 500 + 1600 = 2100  rejected: e-book unit quota (5 left)
        {"AB01", 3},  // 300 + 300 = 600  ok
        {"AB01", 3},  // 600  rejected: audiobook unit quota (1 seat left)
        {"T001", 2},  // free  ok
        {"T001", 4},  // free  rejected: thesis unit quota (3 left)
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource *r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";
    // Q6: taxes. Print items 5%, electronic items 18%. Quotas are checked
    // against the post-tax cost.
    std::cout << "\n=== Taxes (print 5%, electronic 18%) ===\n";
    Catalog taxCatalog;
    taxCatalog.emplace<Book>("TB1", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                             "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    taxCatalog.emplace<ElectronicResource>("TR1", "IEEE Xplore Digital Library", "IEEE", 2026,
                                           Money::of(150), "https://ieeexplore.example",
                                           LicenseModel::AnnualSubscription, Money::of(2000));
    Budget taxBudget(Money::of(10000));
    taxBudget.setQuota(ResourceCategory::Book, {10, Money::of(920)});
    AcquisitionManager taxed(taxCatalog, taxBudget, TaxPolicy(5, 18));
    std::cout << "2 copies of TB1: pre-tax " << taxed.quote("TB1", 2) << ", post-tax "
              << taxed.quoteWithTax("TB1", 2) << "\n";
    taxed.processBatch({
        {"TB1", 2},  // 900 + 45 = 945  rejected: fits 920 before tax, not after
        {"TB1", 1},  // 450 + 22.50 = 472.50  ok
        {"TR1", 20}, // 5000 + 900 = 5900  ok
    });
    taxed.printReport(std::cout);
    taxBudget.print(std::cout);
    // Q7: a limit on the number of different titles per category.
    std::cout << "\n=== Title limit (Book quota: at most 2 different titles) ===\n";
    Catalog titleCatalog;
    for (const char *id : {"TL1", "TL2", "TL3"})
        titleCatalog.emplace<Book>(id, std::string("Book ") + id,
                                   std::vector<std::string>{"A. Author"}, "978-0000000000",
                                   "Publisher", 2020, Money::of(100));
    Budget titleBudget(Money::of(10000));
    titleBudget.setQuota(ResourceCategory::Book, {50, Money::of(5000), 2});
    AcquisitionManager titleAcq(titleCatalog, titleBudget);
    titleAcq.processBatch({
        {"TL1", 1}, // new title (1 of 2)  ok
        {"TL2", 1}, // new title (2 of 2)  ok
        {"TL3", 1}, // third title         rejected: title quota
        {"TL1", 3}, // title already bought  ok
    });
    titleAcq.printReport(std::cout);
    titleBudget.print(std::cout);

    // Q8: cancelling an approved order.
    std::cout << "\n=== Cancellation (Book quota: at most 2 titles, print tax 5%) ===\n";
    Catalog cancelCatalog;
    for (const char *id : {"CB1", "CB2", "CB3"})
        cancelCatalog.emplace<Book>(id, std::string("Book ") + id,
                                    std::vector<std::string>{"A. Author"}, "978-0000000000",
                                    "Publisher", 2020, Money::of(100));
    Budget cancelBudget(Money::of(10000));
    cancelBudget.setQuota(ResourceCategory::Book, {50, Money::of(5000), 2});
    AcquisitionManager cancelAcq(cancelCatalog, cancelBudget, TaxPolicy(5, 0));
    cancelAcq.processBatch({
        {"CB1", 2}, // 200 + 10 = 210  ok          (title 1 of 2)
        {"CB2", 3}, // 300 + 15 = 315  ok          (title 2 of 2)
        {"CB3", 1}, // rejected: title quota
    });
    cancelAcq.cancel(1); // refunds 210, frees CB1's title slot
    cancelAcq.processBatch({
        {"CB3", 1}, // 100 + 5 = 105  ok now that CB1's slot is free
    });
    cancelAcq.printReport(std::cout);
    cancelBudget.print(std::cout);
    std::cout << "Holdings: CB1=" << cancelCatalog.holdings("CB1")
              << " CB2=" << cancelCatalog.holdings("CB2")
              << " CB3=" << cancelCatalog.holdings("CB3") << "\n";

    // Q9: department budgets. Each department has its own budget and quotas.
    std::cout << "\n=== Departments ===\n";
    Catalog deptCatalog;
    for (const char *id : {"DB1", "DB2"})
        deptCatalog.emplace<Book>(id, std::string("Book ") + id,
                                  std::vector<std::string>{"A. Author"}, "978-0000000000",
                                  "Publisher", 2020, Money::of(100));
    const Department cs("Computer Science"), physics("Physics");
    Budget csBudget(Money::of(5000));
    csBudget.setQuota(ResourceCategory::Book, {20, Money::of(3000)});
    Budget physicsBudget(Money::of(1000));
    physicsBudget.setQuota(ResourceCategory::Book, {10, Money::of(500), 1}); // at most 1 title
    AcquisitionManager deptAcq(deptCatalog);
    deptAcq.addDepartment(cs, csBudget);
    deptAcq.addDepartment(physics, physicsBudget);
    deptAcq.processBatch({
        {"DB1", 4, cs},                      // 400  ok
        {"DB1", 3, physics},                 // 300  ok
        {"DB2", 1, physics},                 // rejected: Physics may buy only 1 title
        {"DB1", 3, physics},                 // 300  rejected: Physics spend quota (200 left)
        {"DB1", 2, Department("Chemistry")}, // rejected: unknown department
        {"DB2", 5, cs},                      // 500  ok
    });
    deptAcq.cancel(2); // refunds Physics' 300 (order #2)
    deptAcq.processBatch({
        {"DB2", 2, physics}, // 200  ok: Physics' title slot is free again
    });
    deptAcq.printReport(std::cout);
    deptAcq.printBudgets(std::cout);
    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try
    {
        acq.purchase("B002", 1);
    }
    catch (const QuotaExceededError &e)
    {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }
    return 0;
}
