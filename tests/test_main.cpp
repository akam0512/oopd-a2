// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do                                                                           \
    {                                                                            \
        ++g_checks;                                                              \
        if (!(cond))                                                             \
        {                                                                        \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)             \
    do                                         \
    {                                          \
        bool thrown_ = false;                  \
        try                                    \
        {                                      \
            (void)(expr);                      \
        }                                      \
        catch (const ExType &)                 \
        {                                      \
            thrown_ = true;                    \
        }                                      \
        catch (...)                            \
        {                                      \
        }                                      \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney()
{
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost()
{
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource &r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);
}

static void testCatalog()
{
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    CHECK(c.size() == 3);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    CHECK(c.where([](const Resource &r)
                  { return r.isDigital(); })
              .size() == 1);

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 2);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget()
{
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());                 // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());                // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty()); // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());   // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));
    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300)); // failed commits changed nothing
}

static void testAcquisition()
{
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto &rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1); // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved); // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}
static void testJournal()
{
    Journal j("J1", "Nature", "1234-5678", "Springer", 2026, Money::of(100), 12);
    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(j.subscriptionYears() == 1);     // default
    CHECK(j.costFor(2) == Money::of(200)); // 100 * 2 copies * 1 year

    Journal j3("J2", "Science", "8765-4321", "AAAS", 2026, Money::of(100), 52, 3);
    CHECK(j3.costFor(2) == Money::of(600)); // 100 * 2 copies * 3 years

    std::ostringstream os;
    os << j;
    CHECK(os.str().find("issn: 1234-5678") != std::string::npos);

    CHECK_THROWS(Journal("J3", "T", "i", "P", 2026, Money::of(100), 12, 0),
                 std::invalid_argument);
    CHECK_THROWS(Journal("J3", "T", "i", "P", 2026, Money::of(100), 12, -1),
                 std::invalid_argument);
    CHECK_THROWS(j.costFor(0), std::invalid_argument);

    Catalog c;
    c.emplace<Journal>("J1", "Nature", "1234-5678", "Springer", 2026, Money::of(100), 12);
    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::Journal, {2, Money::of(1000)});
    AcquisitionManager acq(c, b);

    const auto &rec = acq.purchase("J1", 1);
    CHECK(rec.approved && rec.cost == Money::of(100));
    CHECK(c.holdings("J1") == 1);
    CHECK_THROWS(acq.purchase("J1", 2), QuotaExceededError);
    CHECK(c.holdings("J1") == 1);
}
static void testEBook()
{
    EBook e("E1", "Clean Code (e)", {"Robert C. Martin"}, "978-0132350884",
            "Prentice Hall", 2008, Money::of(10), "https://ebooks.example/e1",
            FileFormat::EPUB, true, LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.category() == ResourceCategory::EBook);
    CHECK(e.isDigital());
    CHECK(e.isbn() == "978-0132350884");
    CHECK(e.format() == FileFormat::EPUB);
    CHECK(e.drmProtected());
    CHECK(e.platformFee() == Money::of(100));
    CHECK(e.costFor(5) == Money::of(150));

    const Resource &r = e;
    CHECK(r.costFor(1) == Money::of(110));
    CHECK(r.category() == ResourceCategory::EBook);

    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);
    CHECK(os.str().find("EPUB") != std::string::npos);
    CHECK(os.str().find("978-0132350884") != std::string::npos);

    CHECK_THROWS(e.costFor(0), std::invalid_argument);
    CHECK_THROWS(EBook("E2", "T", {}, "i", "P", 2020, Money::of(10), "url",
                       FileFormat::PDF, false, LicenseModel::Perpetual, Money::fromMinor(-1)),
                 std::invalid_argument);

    Catalog c;
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    c.emplace<EBook>("E1", "Book", std::vector<std::string>{"A"}, "i", "P", 2026,
                     Money::of(10), "url", FileFormat::PDF, false,
                     LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::EBook, {5, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.purchase("E1", 3).approved);
    CHECK_THROWS(acq.purchase("E1", 3), QuotaExceededError);
    CHECK(acq.purchase("R1", 100).approved);
    CHECK(b.usageFor(ResourceCategory::EBook).units == 3);
    CHECK(b.usageFor(ResourceCategory::ElectronicResource).units == 100);
}
static void testAudioBook()
{
    AudioBook a("A1", "Project Hail Mary", "Ballantine", 2021, Money::of(10),
                "https://audio.example/a1", LicenseModel::AnnualSubscription,
                Money::of(100), "Ray Porter", 480);
    CHECK(a.category() == ResourceCategory::AudioBook);
    CHECK(a.isDigital());
    CHECK(a.narrator() == "Ray Porter");
    CHECK(a.durationMinutes() == 480);
    CHECK(a.costFor(5) == Money::of(150));

    const Resource &r = a;
    CHECK(r.costFor(1) == Money::of(110));

    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);
    CHECK(os.str().find("Ray Porter") != std::string::npos);
    CHECK(os.str().find("480") != std::string::npos);

    // Group 2: bad input
    CHECK_THROWS(a.costFor(0), std::invalid_argument);
    CHECK_THROWS(AudioBook("A2", "T", "P", 2020, Money::of(10), "url",
                           LicenseModel::Perpetual, Money{}, "N", 0),
                 std::invalid_argument);
    CHECK_THROWS(AudioBook("A2", "T", "P", 2020, Money::of(10), "url",
                           LicenseModel::Perpetual, Money{}, "N", -5),
                 std::invalid_argument);

    // Group 3: own quota through the acquisition manager
    Catalog c;
    c.emplace<AudioBook>("A1", "Project Hail Mary", "Ballantine", 2021, Money::of(10),
                         "https://audio.example/a1", LicenseModel::AnnualSubscription,
                         Money::of(100), "Ray Porter", 480);
    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::AudioBook, {4, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.purchase("A1", 3).approved);
    CHECK_THROWS(acq.purchase("A1", 2), QuotaExceededError);
    CHECK(b.usageFor(ResourceCategory::AudioBook).units == 3);
    CHECK(c.holdings("A1") == 3);
}

static void testThesis()
{

    Thesis t("T1", "Deep Learning for Records", "IIT Delhi", DegreeLevel::PhD,
             "Dr. Rao", 2025);
    CHECK(t.category() == ResourceCategory::Thesis);
    CHECK(!t.isDigital());
    CHECK(t.unitPrice() == Money{});
    CHECK(t.costFor(3) == Money{});
    CHECK(t.university() == "IIT Delhi");
    CHECK(t.degree() == DegreeLevel::PhD);
    CHECK(t.supervisor() == "Dr. Rao");

    Thesis priced("T2", "Another", "IIT Delhi", DegreeLevel::Master, "Dr. Rao", 2024,
                  Money::of(50));
    CHECK(priced.costFor(2) == Money::of(100));
    std::ostringstream os;
    os << t;
    CHECK(os.str().find("degree: PhD") != std::string::npos);
    CHECK(os.str().find("supervisor: Dr. Rao") != std::string::npos);
    CHECK(os.str().find("IIT Delhi") != std::string::npos);

    // Group 2: bad input
    CHECK_THROWS(t.costFor(0), std::invalid_argument);
    CHECK_THROWS(Thesis("T3", "T", "U", DegreeLevel::Bachelor, "", 2020),
                 std::invalid_argument);
    CHECK_THROWS(Thesis("T3", "T", "U", DegreeLevel::Bachelor, "S", 2020,
                        Money::fromMinor(-1)),
                 std::invalid_argument);
    Catalog c;
    c.emplace<Thesis>("T1", "Deep Learning for Records", "IIT Delhi", DegreeLevel::PhD,
                      "Dr. Rao", 2025);
    Budget b(Money::of(100));
    b.setQuota(ResourceCategory::Thesis, {2, Money::of(0)});
    AcquisitionManager acq(c, b);

    const auto &rec = acq.purchase("T1", 1);
    CHECK(rec.approved && rec.cost == Money{});
    CHECK(c.holdings("T1") == 1);
    CHECK(b.spent() == Money{});
    CHECK_THROWS(acq.purchase("T1", 2), QuotaExceededError);
    CHECK(b.usageFor(ResourceCategory::Thesis).units == 1);
}
static void testHardcover()
{
    Book paper("B1", "T", {"A"}, "i", "P", 2020, Money::of(100));
    Book hard("B2", "T", {"A"}, "i", "P", 2020, Money::of(100), 1, Binding::Hardcover);

    CHECK(paper.costFor(3) == Money::of(300)); // paperback unchanged
    CHECK(hard.costFor(1) == Money::of(120));  // +20%
    CHECK(hard.costFor(3) == Money::of(360));
    CHECK(hard.unitPrice() == Money::of(100)); // listed price is unchanged

    // Rounding: 0.03 * 1.20 = 0.036 -> 0.04 (half away from zero)
    Book tiny("B3", "T", {"A"}, "i", "P", 2020, Money::fromMinor(3), 1, Binding::Hardcover);
    CHECK(tiny.costFor(1) == Money::fromMinor(4));

    CHECK_THROWS(hard.costFor(0), std::invalid_argument);

    // Quota and budget use the surcharged cost
    Catalog c;
    c.emplace<Book>("B2", "T", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(60), 1, Binding::Hardcover);
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(100)});
    AcquisitionManager acq(c, b);
    CHECK(acq.quote("B2", 1) == Money::of(72)); // 60 + 20%
    CHECK(acq.purchase("B2", 1).cost == Money::of(72));
    CHECK_THROWS(acq.purchase("B2", 1), QuotaExceededError); // 72 + 72 > 100
}

static void testBulkDiscounts()
{
    // Print: 10% off at 10+ copies in one order
    Book paper("B1", "T", {"A"}, "i", "P", 2020, Money::of(100));
    CHECK(paper.costFor(9) == Money::of(900));   // no discount
    CHECK(paper.costFor(10) == Money::of(900));  // 1000 - 10%
    CHECK(paper.costFor(20) == Money::of(1800)); // 2000 - 10%

    // Hardcover surcharge first, then the discount: 120 * 10 = 1200 -> 1080
    Book hard("B2", "T", {"A"}, "i", "P", 2020, Money::of(100), 1, Binding::Hardcover);
    CHECK(hard.costFor(10) == Money::of(1080));

    // Rounding: 11 * 0.01 = 0.11 -> 0.099 -> 0.10
    Book cheap("B3", "T", {"A"}, "i", "P", 2020, Money::fromMinor(1));
    CHECK(cheap.costFor(11) == Money::fromMinor(10));

    // Journal: the threshold counts copies, not years
    Journal j("J1", "T", "i", "P", 2026, Money::of(100), 12, 2);
    CHECK(j.costFor(9) == Money::of(1800));  // 9 * 100 * 2
    CHECK(j.costFor(10) == Money::of(1800)); // 2000 - 10%

    // Thesis
    Thesis th("T1", "T", "U", DegreeLevel::Master, "S", 2024, Money::of(10));
    CHECK(th.costFor(9) == Money::of(90));
    CHECK(th.costFor(10) == Money::of(90)); // 100 - 10%

    // Electronic: seats beyond the 50th cost half price; platform fee unchanged
    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.costFor(50) == Money::of(600));  // 100 + 50 * 10
    CHECK(e.costFor(51) == Money::of(605));  // + one seat at 5
    CHECK(e.costFor(60) == Money::of(650));  // + 10 seats at 5
    CHECK(e.costFor(100) == Money::of(850)); // + 50 seats at 5

    // EBook and AudioBook inherit the rule
    EBook eb("E1", "T", {"A"}, "i", "P", 2020, Money::of(10), "url", FileFormat::PDF,
             false, LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(eb.costFor(60) == Money::of(650));
    AudioBook ab("A1", "T", "P", 2020, Money::of(10), "url",
                 LicenseModel::AnnualSubscription, Money::of(100), "N", 60);
    CHECK(ab.costFor(60) == Money::of(650));

    // Quotas and budget use the discounted cost
    Catalog c;
    c.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money{});
    Budget b(Money::of(5000));
    b.setQuota(ResourceCategory::Book, {20, Money::of(950)});
    b.setQuota(ResourceCategory::ElectronicResource, {100, Money::of(560)});
    AcquisitionManager acq(c, b);
    CHECK(acq.purchase("B1", 10).cost == Money::of(900)); // 1000 undiscounted would not fit 950
    CHECK(acq.purchase("R1", 60).cost == Money::of(550)); // 600 undiscounted would not fit 560
}
static void testTaxes()
{
    // Group 1: TaxPolicy on its own
    TaxPolicy none;
    CHECK(none.printPercent() == 0 && none.electronicPercent() == 0); // default: no tax

    TaxPolicy tp(5, 18);
    CHECK(tp.printPercent() == 5 && tp.electronicPercent() == 18);
    tp.setPrintPercent(12);
    CHECK(tp.printPercent() == 12);
    tp.setPrintPercent(5);
    CHECK_THROWS(TaxPolicy(-1, 0), std::invalid_argument);
    CHECK_THROWS(TaxPolicy(0, 101), std::invalid_argument);
    CHECK_THROWS(tp.setElectronicPercent(-5), std::invalid_argument);

    Book book("B1", "T", {"A"}, "i", "P", 2020, Money::of(100));
    Journal journal("J1", "T", "i", "P", 2026, Money::of(100), 12);
    Thesis thesis("T1", "T", "U", DegreeLevel::Master, "S", 2024);
    ElectronicResource eres("R1", "DB", "P", 2026, Money::of(10), "url");
    EBook ebook("E1", "T", {"A"}, "i", "P", 2020, Money::of(10), "url", FileFormat::PDF,
                false);
    AudioBook audio("A1", "T", "P", 2020, Money::of(10), "url",
                    LicenseModel::AnnualSubscription, Money{}, "N", 60);
    CHECK(tp.rateFor(book) == 5 && tp.rateFor(journal) == 5 && tp.rateFor(thesis) == 5);
    CHECK(tp.rateFor(eres) == 18 && tp.rateFor(ebook) == 18 && tp.rateFor(audio) == 18);
    CHECK(tp.taxOn(book, Money::of(200)) == Money::of(10));             // 5% of 200
    CHECK(tp.taxOn(eres, Money::of(200)) == Money::of(36));             // 18% of 200
    CHECK(tp.taxOn(book, Money::fromMinor(10)) == Money::fromMinor(1)); // 0.005 -> 0.01

    // Group 2: AcquisitionManager with taxes
    Catalog c;
    c.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(100));
    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b, TaxPolicy(5, 18));
    CHECK(acq.quote("B1", 2) == Money::of(200));        // quote() is pre-tax
    CHECK(acq.quoteWithTax("B1", 2) == Money::of(210)); // 200 + 5%

    const auto &rec = acq.purchase("B1", 2);
    CHECK(rec.preTaxCost == Money::of(200) && rec.tax == Money::of(10));
    CHECK(rec.cost == Money::of(210));  // post-tax
    CHECK(b.spent() == Money::of(210)); // budget is charged post-tax
    CHECK(c.holdings("B1") == 2);

    const auto &rec2 = acq.purchase("R1", 10); // 100 + 10 * 10 = 200
    CHECK(rec2.preTaxCost == Money::of(200) && rec2.tax == Money::of(36));
    CHECK(rec2.cost == Money::of(236));

    CHECK(acq.totalSpentPreTax() == Money::of(400));
    CHECK(acq.totalTax() == Money::of(46));
    CHECK(acq.totalSpent() == Money::of(446));
    CHECK(b.spent() == acq.totalSpent());

    std::ostringstream os;
    acq.printReport(os);
    CHECK(os.str().find("Total pre-tax: 400.00") != std::string::npos);
    CHECK(os.str().find("Total tax: 46.00") != std::string::npos);
    CHECK(os.str().find("Total spent (post-tax): 446.00") != std::string::npos);

    // The tax rates can be changed later
    acq.setTaxPolicy(TaxPolicy(10, 0));
    CHECK(acq.quoteWithTax("B1", 2) == Money::of(220));
    CHECK(acq.quoteWithTax("R1", 10) == Money::of(200));

    // Group 3: quotas and the budget are checked against the post-tax cost
    Catalog c2;
    c2.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b2(Money::of(10000));
    b2.setQuota(ResourceCategory::Book, {10, Money::of(205)}); // 200 fits, 210 does not
    AcquisitionManager taxed(c2, b2, TaxPolicy(5, 18));
    std::string why;
    CHECK(!taxed.canPurchase("B1", 2, &why));
    CHECK(why.find("spend quota") != std::string::npos);
    CHECK_THROWS(taxed.purchase("B1", 2), QuotaExceededError);
    CHECK(b2.spent() == Money{} && c2.holdings("B1") == 0); // nothing changed
    CHECK(taxed.history().empty());

    AcquisitionManager untaxed(c2, b2); // same quota, no tax
    CHECK(untaxed.canPurchase("B1", 2));
    const auto &free = untaxed.purchase("B1", 2);
    CHECK(free.tax == Money{} && free.cost == free.preTaxCost); // default rates are 0

    Catalog c3;
    c3.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b3(Money::of(205)); // overall budget, no quotas
    AcquisitionManager small(c3, b3, TaxPolicy(5, 18));
    CHECK_THROWS(small.purchase("B1", 2), BudgetExceededError); // 210 > 205
}
static void testTitleLimit()
{
    // Group 1: Budget on its own
    Budget b(Money::of(10000));
    b.setQuota(ResourceCategory::Book, {100, Money::of(5000), 2}); // at most 2 different titles
    CHECK(b.titlesRemaining(ResourceCategory::Book) == 2);

    b.commit(ResourceCategory::Book, 1, Money::of(10), "B1");
    b.commit(ResourceCategory::Book, 1, Money::of(10), "B2");
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);
    CHECK(b.titlesRemaining(ResourceCategory::Book) == 0);

    b.commit(ResourceCategory::Book, 3, Money::of(30), "B1"); // same title again: allowed
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);
    CHECK(b.usageFor(ResourceCategory::Book).units == 5);

    CHECK(b.check(ResourceCategory::Book, 1, Money::of(10), "B1").empty());
    const std::string why = b.check(ResourceCategory::Book, 1, Money::of(10), "B3");
    CHECK(why.find("title quota") != std::string::npos); // a third title is rejected
    CHECK_THROWS(b.commit(ResourceCategory::Book, 1, Money::of(10), "B3"), QuotaExceededError);
    CHECK(b.usageFor(ResourceCategory::Book).units == 5); // failed commit changed nothing
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);

    // Other categories are counted separately, and are unlimited unless a limit is set
    b.setQuota(ResourceCategory::Journal, {100, Money::of(5000)});
    CHECK(!b.titlesRemaining(ResourceCategory::Journal).has_value());
    b.commit(ResourceCategory::Journal, 1, Money::of(10), "J1");
    b.commit(ResourceCategory::Journal, 1, Money::of(10), "J2");
    b.commit(ResourceCategory::Journal, 1, Money::of(10), "J3");
    CHECK(b.usageFor(ResourceCategory::Journal).titles() == 3);
    CHECK(!b.titlesRemaining(ResourceCategory::ElectronicResource).has_value()); // no quota at all

    // Callers that give no title id are not title-limited (old behaviour)
    b.commit(ResourceCategory::Book, 1, Money::of(10));
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);

    // A limit of 0 means no title may be bought; negative limits are invalid
    Budget zero(Money::of(1000));
    zero.setQuota(ResourceCategory::Book, {10, Money::of(1000), 0});
    CHECK(!zero.check(ResourceCategory::Book, 1, Money::of(1), "B1").empty());
    CHECK_THROWS(zero.setQuota(ResourceCategory::Book, {10, Money::of(1000), -1}),
                 std::invalid_argument);

    // Group 2: through the AcquisitionManager
    Catalog c;
    for (const char *id : {"B1", "B2", "B3"})
        c.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(10));
    Budget b2(Money::of(10000));
    b2.setQuota(ResourceCategory::Book, {100, Money::of(5000), 2});
    AcquisitionManager acq(c, b2);

    CHECK(acq.purchase("B1", 2).approved);
    CHECK(acq.purchase("B2", 1).approved);
    std::string reason;
    CHECK(!acq.canPurchase("B3", 1, &reason));
    CHECK(reason.find("title quota") != std::string::npos);
    CHECK_THROWS(acq.purchase("B3", 1), QuotaExceededError);
    CHECK(c.holdings("B3") == 0);
    CHECK(acq.purchase("B1", 5).approved); // existing title: fine

    const auto res = acq.processBatch({{"B3", 1}, {"B2", 1}});
    CHECK(!res[0].approved && res[0].reason.find("title quota") != std::string::npos);
    CHECK(res[1].approved);
}

int main()
{
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testJournal();
    testEBook();
    testAudioBook();
    testThesis();
    testHardcover();
    testBulkDiscounts();
    testTaxes();
    testTitleLimit();
    std::cout
        << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
