// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <map>

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
static void testBudgetRefund()
{
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {10, Money::of(500), 2}); // at most 2 titles
    b.commit(ResourceCategory::Book, 2, Money::of(100), "B1");
    b.commit(ResourceCategory::Book, 3, Money::of(150), "B2");
    CHECK(b.spent() == Money::of(250));
    CHECK(b.usageFor(ResourceCategory::Book).units == 5);
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(10), "B3").empty()); // title limit reached

    // Refunding part of a title: money and units come back, the title still counts
    b.refund(ResourceCategory::Book, 1, Money::of(50), "B1");
    CHECK(b.spent() == Money::of(200));
    CHECK(b.usageFor(ResourceCategory::Book).units == 4);
    CHECK(b.usageFor(ResourceCategory::Book).spent == Money::of(200));
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 2);
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(10), "B3").empty()); // still blocked

    // Refunding the rest of the title: it stops counting, so a new title fits
    b.refund(ResourceCategory::Book, 1, Money::of(50), "B1");
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 1);
    CHECK(b.titlesRemaining(ResourceCategory::Book) == 1);
    CHECK(b.check(ResourceCategory::Book, 1, Money::of(10), "B3").empty());
    CHECK(b.spent() == Money::of(150));
    CHECK(b.remaining() == Money::of(850));
    CHECK(b.unitsRemaining(ResourceCategory::Book) == 7);
    CHECK(b.spendRemaining(ResourceCategory::Book) == Money::of(350));

    // A refund larger than what was bought is rejected and changes nothing
    CHECK_THROWS(b.refund(ResourceCategory::Book, 4, Money::of(10), "B2"), std::invalid_argument);
    CHECK_THROWS(b.refund(ResourceCategory::Book, 1, Money::of(10), "B9"), std::invalid_argument);
    CHECK_THROWS(b.refund(ResourceCategory::Book, 1, Money::of(9999), "B2"), std::invalid_argument);
    CHECK_THROWS(b.refund(ResourceCategory::Book, 0, Money::of(10), "B2"), std::invalid_argument);
    CHECK_THROWS(b.refund(ResourceCategory::Book, 1, Money::fromMinor(-1), "B2"),
                 std::invalid_argument);
    CHECK_THROWS(b.refund(ResourceCategory::Journal, 1, Money::of(10), "J1"),
                 std::invalid_argument); // nothing bought there
    CHECK(b.spent() == Money::of(150));
    CHECK(b.usageFor(ResourceCategory::Book).units == 3);
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 1);

    // Free purchases can be refunded as well
    Budget f(Money::of(10));
    f.commit(ResourceCategory::Thesis, 2, Money{}, "T1");
    f.refund(ResourceCategory::Thesis, 2, Money{}, "T1");
    CHECK(f.usageFor(ResourceCategory::Thesis).units == 0);
    CHECK(f.usageFor(ResourceCategory::Thesis).titles() == 0);

    // Refunded room can be used again
    Budget r(Money::of(100));
    r.setQuota(ResourceCategory::Book, {2, Money::of(100)});
    r.commit(ResourceCategory::Book, 2, Money::of(100), "B1");
    CHECK(!r.check(ResourceCategory::Book, 1, Money::of(1), "B1").empty()); // quota and budget full
    r.refund(ResourceCategory::Book, 2, Money::of(100), "B1");
    r.commit(ResourceCategory::Book, 2, Money::of(100), "B1");
    CHECK(r.spent() == Money::of(100));
}
static void testCancel()
{
    // Setup: books at 100 each, Book quota of at most 2 titles, 10% print tax
    Catalog c;
    for (const char *id : {"B1", "B2", "B3"})
        c.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b(Money::of(10000));
    b.setQuota(ResourceCategory::Book, {100, Money::of(5000), 2});
    AcquisitionManager acq(c, b, TaxPolicy(10, 0));

    CHECK(acq.purchase("B1", 2).cost == Money::of(220)); // order #1: 200 + 10%
    CHECK(acq.purchase("B2", 3).cost == Money::of(330)); // order #2
    CHECK(b.spent() == Money::of(550));
    CHECK(!acq.canPurchase("B3", 1)); // title limit reached

    // Cancel order #1
    const PurchaseRecord cr = acq.cancel(1); // copy: history may grow
    CHECK(cr.cancellation && cr.cancelsOrderNo == 1 && cr.orderNo == 3);
    CHECK(!cr.approved);
    CHECK(cr.resourceId == "B1" && cr.quantity == 2);
    CHECK(cr.cost == Money::of(-220));
    CHECK(cr.preTaxCost == Money::of(-200) && cr.tax == Money::of(-20));
    CHECK(b.spent() == Money::of(330));                   // budget refunded
    CHECK(b.usageFor(ResourceCategory::Book).units == 3); // quota usage refunded
    CHECK(b.usageFor(ResourceCategory::Book).spent == Money::of(330));
    CHECK(c.holdings("B1") == 0 && c.holdings("B2") == 3);   // holdings reduced
    CHECK(b.usageFor(ResourceCategory::Book).titles() == 1); // B1 no longer counted
    CHECK(acq.canPurchase("B3", 1));                         // so a new title fits

    // The original record stays in the history, unchanged
    CHECK(acq.history().size() == 3);
    CHECK(acq.history()[0].orderNo == 1 && acq.history()[0].approved);
    CHECK(!acq.history()[0].cancellation && acq.history()[0].cost == Money::of(220));
    CHECK(acq.history()[2].cancellation);
    CHECK(acq.isCancelled(1) && !acq.isCancelled(2));

    CHECK(acq.purchase("B3", 1).approved);                     // order #4
    CHECK(acq.processBatch({{"X9", 1}})[0].approved == false); // order #5: rejected

    // Orders that cannot be cancelled: nothing changes
    CHECK_THROWS(acq.cancel(1), std::invalid_argument);  // already cancelled
    CHECK_THROWS(acq.cancel(3), std::invalid_argument);  // a cancellation record
    CHECK_THROWS(acq.cancel(5), std::invalid_argument);  // a rejected order
    CHECK_THROWS(acq.cancel(99), std::invalid_argument); // no such order
    CHECK_THROWS(acq.cancel(0), std::invalid_argument);
    CHECK(acq.history().size() == 5);
    CHECK(b.spent() == Money::of(440)); // 330 + 110
    CHECK(c.holdings("B1") == 0 && c.holdings("B2") == 3 && c.holdings("B3") == 1);

    // The same title bought twice: cancelling one order keeps the title counted
    Catalog c2;
    c2.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b2(Money::of(10000));
    b2.setQuota(ResourceCategory::Book, {100, Money::of(5000), 2});
    AcquisitionManager acq2(c2, b2);
    acq2.purchase("B1", 2); // order #1
    acq2.purchase("B1", 3); // order #2
    acq2.cancel(1);
    CHECK(b2.usageFor(ResourceCategory::Book).titles() == 1); // B1 still held
    CHECK(b2.usageFor(ResourceCategory::Book).units == 3);
    CHECK(c2.holdings("B1") == 3);
    acq2.cancel(2);
    CHECK(b2.usageFor(ResourceCategory::Book).titles() == 0); // now it is gone
    CHECK(b2.usageFor(ResourceCategory::Book).units == 0);
    CHECK(b2.spent() == Money{} && c2.holdings("B1") == 0);

    // The refund is the amount stored in the order, even if the tax rate changed
    Catalog c3;
    c3.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b3(Money::of(10000));
    AcquisitionManager acq3(c3, b3, TaxPolicy(10, 0));
    acq3.purchase("B1", 1); // 110
    acq3.setTaxPolicy(TaxPolicy(50, 0));
    acq3.cancel(1);
    CHECK(b3.spent() == Money{});

    // Cancelled capacity can be used again
    Catalog c4;
    c4.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(10));
    Budget b4(Money::of(1000));
    b4.setQuota(ResourceCategory::Book, {2, Money::of(1000)});
    AcquisitionManager acq4(c4, b4);
    acq4.purchase("B1", 2);
    CHECK(!acq4.canPurchase("B1", 1)); // unit quota full
    acq4.cancel(1);
    CHECK(acq4.canPurchase("B1", 2));
    CHECK(acq4.purchase("B1", 2).approved);

    // Free orders can be cancelled too
    Catalog c5;
    c5.emplace<Thesis>("T1", "T", "U", DegreeLevel::Master, "S", 2024);
    Budget b5(Money::of(10));
    AcquisitionManager acq5(c5, b5);
    acq5.purchase("T1", 2);
    CHECK(acq5.cancel(1).cost == Money{});
    CHECK(c5.holdings("T1") == 0 && b5.usageFor(ResourceCategory::Thesis).titles() == 0);

    // If the resource was removed from the catalog, cancel fails and changes nothing
    Catalog c6;
    c6.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(10));
    Budget b6(Money::of(1000));
    AcquisitionManager acq6(c6, b6);
    acq6.purchase("B1", 1);
    c6.remove("B1");
    CHECK_THROWS(acq6.cancel(1), NotFoundError);
    CHECK(b6.spent() == Money::of(10) && !acq6.isCancelled(1) && acq6.history().size() == 1);
}
static void testCancelReport()
{
    Catalog c;
    for (const char *id : {"B1", "B2"})
        c.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget b(Money::of(10000));
    AcquisitionManager acq(c, b, TaxPolicy(10, 0));

    acq.purchase("B1", 2); // order #1: 200 + 20
    acq.purchase("B2", 3); // order #2: 300 + 30
    CHECK(acq.totalSpentPreTax() == Money::of(500));
    CHECK(acq.totalTax() == Money::of(50));
    CHECK(acq.totalSpent() == Money::of(550));

    acq.cancel(1);                                   // order #3
    CHECK(acq.totalSpentPreTax() == Money::of(300)); // totals are net of the refund
    CHECK(acq.totalTax() == Money::of(30));
    CHECK(acq.totalSpent() == Money::of(330));
    CHECK(acq.totalSpent() == b.spent()); // still matches the budget
    CHECK(acq.totalSpentPreTax() + acq.totalTax() == acq.totalSpent());

    acq.processBatch({{"X9", 1}});             // order #4: rejected
    CHECK(acq.totalSpent() == Money::of(330)); // rejected orders never count

    std::ostringstream os;
    acq.printReport(os);
    const std::string report = os.str();
    CHECK(report.find("Order history (4 orders)") != std::string::npos);
    CHECK(report.find("#1   APPROVED  B1") != std::string::npos); // the original is still shown
    CHECK(report.find("#3   CANCELLED B1") != std::string::npos);
    CHECK(report.find("-220.00") != std::string::npos);
    CHECK(report.find("(cancels order #1)") != std::string::npos);
    CHECK(report.find("#4   REJECTED  X9") != std::string::npos);
    CHECK(report.find("Total pre-tax: 300.00") != std::string::npos);
    CHECK(report.find("Total tax: 30.00") != std::string::npos);
    CHECK(report.find("Total spent (post-tax): 330.00") != std::string::npos);

    acq.cancel(2); // cancel everything
    CHECK(acq.totalSpent() == Money{} && acq.totalSpentPreTax() == Money{});
    CHECK(acq.totalTax() == Money{} && b.spent() == Money{});
}
static void testDepartment()
{
    Department cs("Computer Science");
    CHECK(cs.name() == "Computer Science");
    CHECK(cs == Department("Computer Science"));
    CHECK(cs != Department("Physics"));
    CHECK(Department("Physics") != Department("physics")); // names are case-sensitive
    CHECK(Department::general().name() == "General");
    CHECK(Department::general() == Department("General"));

    CHECK_THROWS(Department(""), std::invalid_argument);
    CHECK_THROWS(Department("   "), std::invalid_argument);

    // Ordered by name, so it works as a map key
    std::map<Department, int> m;
    m[Department("Physics")] = 1;
    m[Department("Chemistry")] = 2;
    m[Department("Physics")] += 10;
    CHECK(m.size() == 2);
    CHECK(m.at(Department("Physics")) == 11);
    CHECK(m.begin()->first == Department("Chemistry")); // Chemistry < Physics

    std::ostringstream os;
    os << cs;
    CHECK(os.str() == "Computer Science");
}
static void testDepartmentBudgets()
{
    Catalog c;
    for (const char *id : {"B1", "B2"})
        c.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget csBudget(Money::of(1000));
    csBudget.setQuota(ResourceCategory::Book, {10, Money::of(800)});
    Budget physBudget(Money::of(300));
    physBudget.setQuota(ResourceCategory::Book, {5, Money::of(250), 1}); // at most 1 title
    const Department cs("Computer Science"), phys("Physics");

    AcquisitionManager acq(c); // no budgets yet
    acq.addDepartment(cs, csBudget);
    acq.addDepartment(phys, physBudget);
    CHECK(acq.departments().size() == 2);
    CHECK(&acq.budgetFor(cs) == &csBudget);

    // A purchase is charged to the department it names, and only to it
    const auto &r1 = acq.purchase("B1", 3, cs);
    CHECK(r1.approved && r1.department == cs && r1.cost == Money::of(300));
    CHECK(csBudget.spent() == Money::of(300) && physBudget.spent() == Money{});
    CHECK(c.holdings("B1") == 3); // holdings are shared
    acq.purchase("B1", 2, phys);
    CHECK(physBudget.spent() == Money::of(200) && csBudget.spent() == Money::of(300));
    CHECK(c.holdings("B1") == 5);

    // Each department has its own quotas
    std::string why;
    CHECK(!acq.canPurchase("B1", 1, phys, &why)); // 200 + 100 > spend quota 250
    CHECK(why.find("spend quota") != std::string::npos);
    CHECK(acq.canPurchase("B1", 1, cs));
    CHECK_THROWS(acq.purchase("B1", 1, phys), QuotaExceededError);
    CHECK(!acq.canPurchase("B2", 1, phys, &why)); // Physics: only 1 title allowed
    CHECK(why.find("title quota") != std::string::npos);
    CHECK(acq.canPurchase("B2", 1, cs)); // CS has no title limit

    // Unknown department
    const Department chem("Chemistry");
    CHECK_THROWS(acq.purchase("B1", 1, chem), UnknownDepartmentError);
    CHECK(!acq.canPurchase("B1", 1, chem, &why));
    CHECK(why == "unknown department: Chemistry");
    CHECK_THROWS(acq.purchase("B1", 1), UnknownDepartmentError); // General is not registered here
    CHECK_THROWS(acq.budgetFor(chem), UnknownDepartmentError);
    CHECK(c.holdings("B1") == 5 && csBudget.spent() == Money::of(300)); // nothing changed

    // Batch: each request names its department (or none: General)
    const auto res = acq.processBatch({
        {"B1", 2, cs},   // ok
        {"B1", 1, phys}, // rejected: Physics spend quota
        {"B1", 1, chem}, // rejected: unknown department
        {"B1", 1},       // no department -> General, not registered here
    });
    CHECK(res[0].approved && res[0].department == cs);
    CHECK(!res[1].approved && res[1].reason.find("spend quota") != std::string::npos);
    CHECK(!res[2].approved && res[2].reason == "unknown department: Chemistry");
    CHECK(!res[3].approved && res[3].reason == "unknown department: General");
    CHECK(res[1].department == phys && res[3].department == Department::general());
    CHECK(csBudget.spent() == Money::of(500));

    // Registering departments: no duplicates, and a Budget cannot be shared
    CHECK_THROWS(acq.addDepartment(cs, physBudget), std::invalid_argument);
    CHECK_THROWS(acq.addDepartment(Department("Math"), csBudget), std::invalid_argument);
    Budget mathBudget(Money::of(10));
    acq.addDepartment(Department("Math"), mathBudget);
    CHECK(acq.departments().size() == 3);

    // The single-budget constructor charges everyone to "General", as before
    Catalog c2;
    c2.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget single(Money::of(1000));
    AcquisitionManager legacy(c2, single);
    CHECK(legacy.purchase("B1", 2).department == Department::general());
    CHECK(single.spent() == Money::of(200));
    CHECK(legacy.canPurchase("B1", 1));
    CHECK(legacy.departments().size() == 1);
    Budget art(Money::of(500));
    legacy.addDepartment(Department("Art"), art); // more can be added later
    legacy.purchase("B1", 1, Department("Art"));
    CHECK(art.spent() == Money::of(100) && single.spent() == Money::of(200));

    // Cancelling refunds the department the order was charged to
    Catalog c3;
    c3.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget csB(Money::of(1000)), physB(Money::of(1000));
    AcquisitionManager acq3(c3);
    acq3.addDepartment(cs, csB);
    acq3.addDepartment(phys, physB);
    acq3.purchase("B1", 2, cs);   // order #1
    acq3.purchase("B1", 3, phys); // order #2
    const PurchaseRecord cr = acq3.cancel(2);
    CHECK(cr.cancellation && cr.department == phys);
    CHECK(physB.spent() == Money{} && csB.spent() == Money::of(200));
    CHECK(c3.holdings("B1") == 2);
    acq3.cancel(1);
    CHECK(csB.spent() == Money{} && c3.holdings("B1") == 0);
}
static void testDepartmentReport()
{
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    Budget csB(Money::of(1000)), physB(Money::of(1000));
    const Department cs("Computer Science"), phys("Physics");
    AcquisitionManager acq(c, TaxPolicy(10, 0));
    acq.addDepartment(cs, csB);
    acq.addDepartment(phys, physB);

    acq.purchase("B1", 2, cs);                        // 200 + 20 = 220
    acq.purchase("B1", 3, phys);                      // 300 + 30 = 330   (order #2)
    acq.cancel(2);                                    // order #3
    acq.processBatch({{"B1", 1, Department("Art")}}); // order #4: rejected

    // Totals per department are net of cancellations
    const DepartmentTotals csT = acq.totalsFor(cs);
    CHECK(csT.preTax == Money::of(200) && csT.tax == Money::of(20) && csT.postTax == Money::of(220));
    const DepartmentTotals physT = acq.totalsFor(phys);
    CHECK(physT.preTax == Money{} && physT.tax == Money{} && physT.postTax == Money{});
    CHECK(csT.postTax + physT.postTax == acq.totalSpent());
    CHECK(csB.spent() == csT.postTax && physB.spent() == physT.postTax);
    CHECK_THROWS(acq.totalsFor(Department("Art")), UnknownDepartmentError);

    std::ostringstream os;
    acq.printReport(os);
    const std::string report = os.str();
    CHECK(report.find("Clean Code  [Computer Science]") != std::string::npos);
    CHECK(report.find("Clean Code  [Physics]  (cancels order #2)") != std::string::npos);
    CHECK(report.find("[Art]") != std::string::npos); // the rejected order names its department
    CHECK(report.find("By department:") != std::string::npos);
    CHECK(report.find("Computer Science") != std::string::npos);
    CHECK(report.find("pre-tax 200.00, tax 20.00, post-tax 220.00") != std::string::npos);
    CHECK(report.find("Total spent (post-tax): 220.00") != std::string::npos);

    std::ostringstream budgets;
    acq.printBudgets(budgets);
    CHECK(budgets.str().find("Department: Computer Science") != std::string::npos);
    CHECK(budgets.str().find("Department: Physics") != std::string::npos);

    // With the single-budget constructor the report looks as it always did
    Catalog c2;
    c2.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"A"}, "i", "P", 2020,
                     Money::of(100));
    Budget single(Money::of(1000));
    AcquisitionManager legacy(c2, single);
    legacy.purchase("B1", 1);
    std::ostringstream legacyOut;
    legacy.printReport(legacyOut);
    CHECK(legacyOut.str().find("[General]") == std::string::npos);
    CHECK(legacyOut.str().find("By department") == std::string::npos);
    CHECK(legacyOut.str().find("Total spent (post-tax): 100.00") != std::string::npos);
}
static void testRollover()
{
    Budget year1(Money::of(1000));
    year1.setQuota(ResourceCategory::Book, {10, Money::of(500), 2});
    year1.setQuota(ResourceCategory::Journal, {3, Money::of(300)});
    year1.commit(ResourceCategory::Book, 4, Money::of(400), "B1");
    year1.commit(ResourceCategory::Journal, 1, Money::of(100), "J1");
    CHECK(year1.remaining() == Money::of(500)); // unspent this year

    // The carried-forward amount is a percentage of the unspent money
    CHECK(year1.carryForwardAmount(50) == Money::of(250));
    CHECK(year1.carryForwardAmount(0) == Money{});
    CHECK(year1.carryForwardAmount(100) == Money::of(500));
    CHECK_THROWS(year1.carryForwardAmount(-1), std::invalid_argument);
    CHECK_THROWS(year1.carryForwardAmount(101), std::invalid_argument);

    // Next year: this year's total as the new allocation, plus the carry-forward
    Budget year2 = year1.rollover(50);
    CHECK(year2.total() == Money::of(1250)); // 1000 + 250
    CHECK(year2.spent() == Money{} && year2.remaining() == Money::of(1250));
    CHECK(year2.quotaFor(ResourceCategory::Book).has_value()); // quotas are kept
    CHECK(year2.quotaFor(ResourceCategory::Book)->maxUnits == 10);
    CHECK(year2.quotaFor(ResourceCategory::Book)->maxSpend == Money::of(500));
    CHECK(year2.quotaFor(ResourceCategory::Book)->maxTitles == 2);
    CHECK(year2.quotaFor(ResourceCategory::Journal)->maxUnits == 3);
    CHECK(!year2.quotaFor(ResourceCategory::Thesis).has_value());
    CHECK(year2.usageFor(ResourceCategory::Book).units == 0); // usage starts again at zero
    CHECK(year2.usageFor(ResourceCategory::Book).titles() == 0);
    CHECK(year2.usageFor(ResourceCategory::Journal).spent == Money{});
    CHECK(year1.spent() == Money::of(500)); // this year is not changed
    CHECK(year1.usageFor(ResourceCategory::Book).units == 4);

    // A different new allocation, and the percentage extremes
    CHECK(year1.rollover(50, Money::of(2000)).total() == Money::of(2250));
    CHECK(year1.rollover(0).total() == Money::of(1000));          // nothing carried forward
    CHECK(year1.rollover(100).total() == Money::of(1500));        // everything carried forward
    CHECK(year1.rollover(25, Money{}).total() == Money::of(125)); // only the carry-forward
    CHECK_THROWS(year1.rollover(101), std::invalid_argument);
    CHECK_THROWS(year1.rollover(-5, Money::of(10)), std::invalid_argument);
    CHECK_THROWS(year1.rollover(50, Money::fromMinor(-1)), std::invalid_argument);

    // Next year's budget works like any other, with fresh quotas
    year2.commit(ResourceCategory::Book, 10, Money::of(500), "B1");
    CHECK(year2.unitsRemaining(ResourceCategory::Book) == 0);
    CHECK(year2.remaining() == Money::of(750));
    CHECK(!year2.check(ResourceCategory::Book, 1, Money::of(1), "B1").empty()); // unit quota full

    // Rounding: 33% of 10.01 is 3.3033, rounded to 3.30
    Budget odd(Money::fromMinor(1001));
    CHECK(odd.carryForwardAmount(33) == Money::fromMinor(330));

    // A fully spent budget carries nothing; refunds count as unspent
    Budget full(Money::of(100));
    full.commit(ResourceCategory::Book, 1, Money::of(100), "B1");
    CHECK(full.rollover(50).total() == Money::of(100));
    full.refund(ResourceCategory::Book, 1, Money::of(100), "B1");
    CHECK(full.rollover(50).total() == Money::of(150));

    // With departments: every department's budget rolls over on its own
    Catalog c;
    c.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    Budget csB(Money::of(1000)), physB(Money::of(500));
    csB.setQuota(ResourceCategory::Book, {10, Money::of(800)});
    const Department cs("Computer Science"), phys("Physics");
    AcquisitionManager acq(c);
    acq.addDepartment(cs, csB);
    acq.addDepartment(phys, physB);
    acq.purchase("B1", 3, cs); // CS spends 300, 700 left
    std::map<Department, Budget> next = acq.rolloverBudgets(50);
    CHECK(next.size() == 2);
    CHECK(next.at(cs).total() == Money::of(1350));  // 1000 + 50% of 700
    CHECK(next.at(phys).total() == Money::of(750)); // 500 + 50% of 500
    CHECK(next.at(cs).quotaFor(ResourceCategory::Book)->maxUnits == 10);
    CHECK(csB.spent() == Money::of(300)); // this year's budgets are unchanged

    AcquisitionManager nextYear(c); // a new manager for the new year
    nextYear.addDepartment(cs, next.at(cs));
    nextYear.addDepartment(phys, next.at(phys));
    CHECK(nextYear.purchase("B1", 1, phys).approved);
    CHECK(next.at(phys).spent() == Money::of(100) && physB.spent() == Money{});
}
static void testAllOrNothing()
{
    // Two books at 100 each; the Book quota allows 5 units
    auto makeCatalog = [](Catalog &c)
    {
        for (const char *id : {"B1", "B2"})
            c.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(100));
    };

    // Default (not all-or-nothing): each request is decided on its own, as before
    Catalog c1;
    makeCatalog(c1);
    Budget b1(Money::of(1000));
    b1.setQuota(ResourceCategory::Book, {5, Money::of(1000)});
    AcquisitionManager normal(c1, b1);
    const auto r1 = normal.processBatch({{"B1", 3}, {"B2", 3}, {"B1", 1}});
    CHECK(r1[0].approved && !r1[1].approved && r1[2].approved);
    CHECK(b1.spent() == Money::of(400));

    // All-or-nothing, every request fits: all are bought
    Catalog c2;
    makeCatalog(c2);
    Budget b2(Money::of(1000));
    b2.setQuota(ResourceCategory::Book, {5, Money::of(1000)});
    AcquisitionManager strict(c2, b2);
    const auto r2 = strict.processBatch({{"B1", 2}, {"B2", 2}}, true);
    CHECK(r2.size() == 2 && r2[0].approved && r2[1].approved);
    CHECK(b2.spent() == Money::of(400) && c2.holdings("B1") == 2 && c2.holdings("B2") == 2);

    // Each request fits alone (3 <= 5 units) but not together: nothing is bought
    Catalog c3;
    makeCatalog(c3);
    Budget b3(Money::of(1000));
    b3.setQuota(ResourceCategory::Book, {5, Money::of(1000)});
    AcquisitionManager strict3(c3, b3);
    const auto r3 = strict3.processBatch({{"B1", 3}, {"B2", 3}}, true);
    CHECK(r3.size() == 2 && !r3[0].approved && !r3[1].approved);
    CHECK(r3[0].reason.find("request #2") != std::string::npos); // says which request failed
    CHECK(r3[1].reason.find("unit quota") != std::string::npos); // and why
    CHECK(b3.spent() == Money{} && b3.usageFor(ResourceCategory::Book).units == 0);
    CHECK(c3.holdings("B1") == 0 && c3.holdings("B2") == 0);
    CHECK(strict3.history().size() == 2); // the rejections are recorded
    CHECK(strict3.totalSpent() == Money{});
    CHECK(strict3.purchase("B1", 5).approved); // the budget was left untouched

    // An unknown resource or a bad quantity rejects the whole batch too
    const auto r4 = strict.processBatch({{"B1", 1}, {"X9", 1}}, true); // 1 unit is left
    CHECK(!r4[0].approved && !r4[1].approved);
    CHECK(r4[0].reason.find("request #2") != std::string::npos);
    CHECK(r4[1].reason == "resource not found: X9");
    CHECK(c2.holdings("B1") == 2); // the first request was not bought
    const auto r5 = strict3.processBatch({{"B2", 0}}, true);
    CHECK(!r5[0].approved && r5[0].reason == "quantity must be positive");

    // The title limit is part of the dry run as well
    Catalog c4;
    for (const char *id : {"B1", "B2", "B3"})
        c4.emplace<Book>(id, "T", std::vector<std::string>{"A"}, "i", "P", 2020, Money::of(10));
    Budget b4(Money::of(1000));
    b4.setQuota(ResourceCategory::Book, {50, Money::of(500), 2});
    AcquisitionManager strict4(c4, b4);
    strict4.processBatch({{"B1", 1}, {"B2", 1}, {"B3", 1}}, true); // three titles, limit two
    CHECK(b4.usageFor(ResourceCategory::Book).titles() == 0 && b4.spent() == Money{});

    // Departments: one department's failure stops the other's purchase too
    Catalog c5;
    makeCatalog(c5);
    Budget csB(Money::of(1000)), physB(Money::of(150));
    const Department cs("Computer Science"), phys("Physics");
    AcquisitionManager dept(c5);
    dept.addDepartment(cs, csB);
    dept.addDepartment(phys, physB);
    dept.processBatch({{"B1", 2, cs}, {"B2", 2, phys}}, true); // Physics can only afford 1
    CHECK(csB.spent() == Money{} && physB.spent() == Money{} && c5.holdings("B1") == 0);
    const auto r6 = dept.processBatch({{"B1", 2, cs}, {"B2", 1, phys}}, true);
    CHECK(r6[0].approved && r6[1].approved && csB.spent() == Money::of(200));

    // An empty batch does nothing
    CHECK(strict.processBatch({}, true).empty());
}
static void testVendors()
{
    // Without offers, the list price is used
    Book book("B1", "T", {"A"}, "i", "P", 2020, Money::of(450));
    CHECK(book.offers().empty() && book.cheapestOffer() == nullptr);
    CHECK(book.purchasePrice() == Money::of(450));

    // Several vendors: the cheapest one sets the price
    book.setOffer("BookWorld", Money::of(420));
    book.setOffer("Readers Hub", Money::of(400));
    book.setOffer("Campus Store", Money::of(430));
    CHECK(book.offers().size() == 3);
    CHECK(book.cheapestOffer()->vendor == "Readers Hub");
    CHECK(book.purchasePrice() == Money::of(400));
    CHECK(book.unitPrice() == Money::of(450)); // the list price is unchanged
    CHECK(book.costFor(2) == Money::of(800));  // 2 x 400

    // Changing a listed vendor's price does not add a second entry
    book.setOffer("Readers Hub", Money::of(480));
    CHECK(book.offers().size() == 3);
    CHECK(book.cheapestOffer()->vendor == "BookWorld");
    CHECK(book.purchasePrice() == Money::of(420));

    // A tie goes to the vendor added first
    Book tie("B2", "T", {"A"}, "i", "P", 2020, Money::of(500));
    tie.setOffer("First", Money::of(100));
    tie.setOffer("Second", Money::of(100));
    CHECK(tie.cheapestOffer()->vendor == "First");

    // Bad input is rejected and changes nothing
    CHECK_THROWS(book.setOffer("", Money::of(10)), std::invalid_argument);
    CHECK_THROWS(book.setOffer("   ", Money::of(10)), std::invalid_argument);
    CHECK_THROWS(book.setOffer("Cheap", Money::fromMinor(-1)), std::invalid_argument);
    CHECK(book.offers().size() == 3);

    // Every kind of resource prices from the cheapest vendor, with its own rules on top
    Book hard("B3", "T", {"A"}, "i", "P", 2020, Money::of(1000), 1, Binding::Hardcover);
    hard.setOffer("V", Money::of(500));
    CHECK(hard.costFor(1) == Money::of(600)); // 500 + 20% hardcover
    Book bulk("B4", "T", {"A"}, "i", "P", 2020, Money::of(200));
    bulk.setOffer("V", Money::of(100));
    CHECK(bulk.costFor(10) == Money::of(900)); // 10 x 100, minus 10% bulk discount
    Journal journal("J1", "T", "i", "P", 2026, Money::of(500), 12, 2);
    journal.setOffer("V", Money::of(100));
    CHECK(journal.costFor(3) == Money::of(600)); // 100 x 3 copies x 2 years
    Thesis thesis("T1", "T", "U", DegreeLevel::Master, "S", 2024);
    CHECK(thesis.costFor(2) == Money{}); // free without vendors
    thesis.setOffer("Binder", Money::of(50));
    CHECK(thesis.costFor(2) == Money::of(100));
    ElectronicResource eres("R1", "DB", "P", 2026, Money::of(100), "url",
                            LicenseModel::AnnualSubscription, Money::of(100));
    eres.setOffer("V", Money::of(10));
    CHECK(eres.costFor(60) == Money::of(650)); // 100 fee + 50 x 10 + 10 x 5
    EBook ebook("E1", "T", {"A"}, "i", "P", 2020, Money::of(100), "url", FileFormat::PDF,
                false, LicenseModel::AnnualSubscription, Money::of(100));
    ebook.setOffer("V", Money::of(10));
    CHECK(ebook.costFor(60) == Money::of(650));
    AudioBook audio("A1", "T", "P", 2020, Money::of(100), "url",
                    LicenseModel::AnnualSubscription, Money::of(100), "N", 60);
    audio.setOffer("V", Money::of(10));
    CHECK(audio.costFor(60) == Money::of(650));

    // Printing lists the vendors
    std::ostringstream os;
    os << book;
    CHECK(os.str().find("unit price: 450.00") != std::string::npos);
    CHECK(os.str().find("vendor: BookWorld @ 420.00") != std::string::npos);

    // Purchases are made from the cheapest vendor, and the order says which
    Catalog c;
    Resource &b = c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"A"}, "i", "P",
                                  2020, Money::of(100));
    c.emplace<Book>("B2", "No Vendors", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    b.setOffer("V1", Money::of(90));
    b.setOffer("V2", Money::of(80));
    b.setOffer("V3", Money::of(85));
    Budget budget(Money::of(10000));
    AcquisitionManager acq(c, budget, TaxPolicy(10, 0));
    CHECK(acq.quote("B1", 2) == Money::of(160));        // quotes use the cheapest vendor
    const PurchaseRecord first = acq.purchase("B1", 2); // order #1
    CHECK(first.vendor == "V2");
    CHECK(first.preTaxCost == Money::of(160) && first.tax == Money::of(16));
    CHECK(first.cost == Money::of(176) && budget.spent() == Money::of(176));
    CHECK(c.holdings("B1") == 2);

    b.setOffer("V2", Money::of(95));                     // V2 raises its price
    const PurchaseRecord second = acq.purchase("B1", 1); // order #2
    CHECK(second.vendor == "V3" && second.preTaxCost == Money::of(85));
    CHECK(acq.history()[0].vendor == "V2"); // the first order keeps its vendor

    CHECK(acq.purchase("B2", 1).vendor.empty()); // no vendors: the list price
    CHECK(acq.purchase("B2", 1).preTaxCost == Money::of(100));
    CHECK(acq.processBatch({{"X9", 1}})[0].vendor.empty()); // a rejected order has no vendor

    // Cancelling refunds what was paid and keeps the vendor on the cancellation record
    b.setOffer("V3", Money::of(10));
    const PurchaseRecord cr = acq.cancel(1);
    CHECK(cr.vendor == "V2" && cr.cost == Money::of(-176));
    CHECK(budget.spent() == Money::fromMinor(31350)); // 489.50 spent, minus the 176.00 refund

    // Quotas use the vendor price: 2 copies cost 160 from V2 but 200 at list price
    Catalog c2;
    Resource &b2 = c2.emplace<Book>("B1", "T", std::vector<std::string>{"A"}, "i", "P", 2020,
                                    Money::of(100));
    b2.setOffer("V2", Money::of(80));
    Budget limited(Money::of(10000));
    limited.setQuota(ResourceCategory::Book, {10, Money::of(170)});
    AcquisitionManager acq2(c2, limited);
    CHECK(acq2.purchase("B1", 2).approved);

    // The report shows the vendor
    std::ostringstream report;
    acq.printReport(report);
    CHECK(report.str().find("from V2") != std::string::npos);
    CHECK(report.str().find("from V3") != std::string::npos);
}
static void testSearches()
{
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                    "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    c.emplace<Book>("B2", "The C++ Programming Language", std::vector<std::string>{"Bjarne Stroustrup"},
                    "978-0321563842", "Addison-Wesley", 2013, Money::of(1200));
    c.emplace<Book>("B3", "Clean Architecture",
                    std::vector<std::string>{"Robert C. Martin", "Another Author"},
                    "978-0134494166", "Pearson", 2017, Money::of(500));
    c.emplace<EBook>("E1", "Effective Modern C++", std::vector<std::string>{"Scott Meyers"},
                     "978-1491903995", "O'Reilly", 2014, Money::of(200), "url", FileFormat::PDF,
                     true);
    c.emplace<Journal>("J1", "Nature", "0028-0836", "Springer", 2026, Money::of(300), 51);
    c.emplace<Journal>("J2", "Science", "1234-567X", "AAAS", 2020, Money::of(300), 52);
    c.emplace<AudioBook>("A1", "Project Hail Mary", "Ballantine", 2021, Money::of(100), "url",
                         LicenseModel::AnnualSubscription, Money{}, "Ray Porter", 480);
    c.emplace<Thesis>("T1", "Deep Learning", "IIT Delhi", DegreeLevel::PhD, "Dr. Rao", 2025);
    c.emplace<ElectronicResource>("R1", "IEEE Xplore", "IEEE", 2026, Money::of(150), "url");

    // By author: part of a name, any letter case, any of several authors
    auto martin = c.searchAuthor("martin");
    CHECK(martin.size() == 2 && martin[0]->id() == "B1" && martin[1]->id() == "B3"); // id order
    CHECK(c.searchAuthor("MARTIN").size() == 2);
    CHECK(c.searchAuthor("Robert C. Martin").size() == 2);
    CHECK(c.searchAuthor("another").size() == 1); // the second author of B3
    CHECK(c.searchAuthor("Stroustrup")[0]->id() == "B2");
    CHECK(c.searchAuthor("Meyers")[0]->id() == "E1"); // e-books have authors too
    CHECK(c.searchAuthor("Ray Porter").empty());      // a narrator is not an author
    CHECK(c.searchAuthor("Dr. Rao").empty());         // nor is a supervisor
    CHECK(c.searchAuthor("nobody").empty());
    CHECK_THROWS(c.searchAuthor(""), std::invalid_argument);

    // By ISBN or ISSN: hyphens, spaces and letter case are ignored
    CHECK(c.searchIsbnIssn("978-0132350884").size() == 1);
    CHECK(c.searchIsbnIssn("978-0132350884")[0]->id() == "B1");
    CHECK(c.searchIsbnIssn("9780132350884")[0]->id() == "B1");
    CHECK(c.searchIsbnIssn("978 0132 350884")[0]->id() == "B1");
    CHECK(c.searchIsbnIssn("978-1491903995")[0]->id() == "E1"); // an e-book ISBN
    CHECK(c.searchIsbnIssn("0028-0836")[0]->id() == "J1");      // an ISSN
    CHECK(c.searchIsbnIssn("00280836")[0]->id() == "J1");
    CHECK(c.searchIsbnIssn("1234-567x")[0]->id() == "J2"); // ISSN ending in X
    CHECK(c.searchIsbnIssn("000-0000000").empty());
    CHECK_THROWS(c.searchIsbnIssn(""), std::invalid_argument);
    CHECK_THROWS(c.searchIsbnIssn("- -"), std::invalid_argument); // nothing left after cleaning

    // By range of publication years: both ends included, any kind of resource
    auto mid = c.searchYears(2010, 2017);
    CHECK(mid.size() == 3 && mid[0]->id() == "B2" && mid[1]->id() == "B3" && mid[2]->id() == "E1");
    CHECK(c.searchYears(2008, 2008).size() == 1); // a single year
    CHECK(c.searchYears(2008, 2008)[0]->id() == "B1");
    CHECK(c.searchYears(2013, 2014).size() == 2);        // both ends are included
    CHECK(c.searchYears(2025, 2026).size() == 3);        // T1, J1, R1
    CHECK(c.searchYears(1900, 2100).size() == c.size()); // everything
    CHECK(c.searchYears(1990, 2000).empty());
    CHECK_THROWS(c.searchYears(2020, 2010), std::invalid_argument);

    // An empty catalog finds nothing
    Catalog empty;
    CHECK(empty.searchAuthor("x").empty() && empty.searchIsbnIssn("1").empty());
    CHECK(empty.searchYears(2000, 2030).empty());
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
    testBudgetRefund();
    testCancel();
    testCancelReport();
    testDepartment();
    testDepartmentBudgets();
    testDepartmentReport();
    testRollover();
    testAllOrNothing();
    testVendors();
    testSearches();
    std::cout
        << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
