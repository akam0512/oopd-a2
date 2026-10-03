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
