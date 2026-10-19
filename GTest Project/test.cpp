#include "pch.h"

#include "../Source/factory.h"


// =============================================================================
//  Helpers
// =============================================================================
namespace {

    constexpr double kEps = 1e-6;

    // Удобный доступ к SalaryInfo по указателю на сотрудника
    const SalaryInfo& infoFor(
        const std::unordered_map<const Employee*, SalaryInfo>& results,
        const Employee* emp)
    {
        auto it = results.find(emp);
        if (it == results.end())
            throw std::runtime_error("salary for employee not found");
        return it->second;
    }

} // namespace

// =============================================================================
//  Worker
// =============================================================================
TEST(Worker, NoYearsWorked_ReturnsBaseSalary) {
    Factory f;
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 30'000.0, kEps);
    EXPECT_NEAR(infoFor(r, w).descendants, 0.0, kEps);
}

TEST(Worker, FiveYearsWorked_AddsFiftyPercent) {
    Factory f;
    Worker* w = f.hireWorker("W", 30'000.0, Date(2021, 1, 1));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 45'000.0, kEps);   // 30000 * 1.5
}

TEST(Worker, TenYearsWorked_DoublesSalary) {
    Factory f;
    Worker* w = f.hireWorker("W", 50'000.0, Date(2016, 1, 1));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 100'000.0, kEps);  // 50000 * 2.0
}

TEST(Worker, MoreThanTenYears_CapsAtDoubleSalary) {
    Factory f;
    Worker* w = f.hireWorker("W", 50'000.0, Date(2006, 1, 1)); // 20 лет

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 100'000.0, kEps);  // надбавка 100 %
}

TEST(Worker, MonthBeforeHireAnniversary_DoesNotCountYear) {
    Factory f;
    Worker* w = f.hireWorker("W", 30'000.0, Date(2021, 6, 15)); // 4 года

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 30'000.0 * 1.4, kEps);
}

TEST(Worker, HiredInSameMonth_HasZeroFullYears) {
    Factory f;
    Worker* w = f.hireWorker("W", 40'000.0, Date(2026, 1, 15));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, w).own, 40'000.0, kEps);
}

// =============================================================================
//  Foreman
// =============================================================================
TEST(Foreman, NoYearsNoSubordinates_ReturnsBaseSalary) {
    Factory f;
    Foreman* fm = f.hireForeman("F", 60'000.0, Date(2026, 1, 1));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, fm).own, 60'000.0, kEps);
    EXPECT_NEAR(infoFor(r, fm).descendants, 0.0, kEps);
}

TEST(Foreman, EightYearsWorked_CapsSeniorityAtFortyPercent) {
    Factory f;
    Foreman* fm = f.hireForeman("F", 50'000.0, Date(2018, 1, 1)); // 8 лет

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, fm).own, 50'000.0 * 1.4, kEps);
}

TEST(Foreman, TwelveYears_StillCappedAtFortyPercent) {
    Factory f;
    Foreman* fm = f.hireForeman("F", 50'000.0, Date(2014, 1, 1)); // 12 лет

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, fm).own, 50'000.0 * 1.4, kEps);
}

TEST(Foreman, AddsSevenPercentOfDirectSubordinatesSalaries) {
    Factory f;
    Foreman* fm = f.hireForeman("F", 60'000.0, Date(2022, 1, 1)); // 4 года → +20 %
    Worker* w1 = f.hireWorker("W1", 30'000.0, Date(2023, 1, 1)); // 3 года → +30 %
    Worker* w2 = f.hireWorker("W2", 40'000.0, Date(2023, 1, 1)); // 3 года → +30 %

    f.setManager(w1, fm);
    f.setManager(w2, fm);

    auto r = f.calculateSalaries(Date(2026, 1, 1));

    const double w1Salary = 30'000.0 * 1.3;                 // 39000
    const double w2Salary = 40'000.0 * 1.3;                 // 52000
    const double expected = 60'000.0 * 1.2 + 0.07 * (w1Salary + w2Salary);

    EXPECT_NEAR(infoFor(r, w1).own, w1Salary, kEps);
    EXPECT_NEAR(infoFor(r, w2).own, w2Salary, kEps);
    EXPECT_NEAR(infoFor(r, fm).own, expected, kEps);
    EXPECT_NEAR(infoFor(r, fm).descendants, w1Salary + w2Salary, kEps);
}

TEST(Foreman, SevenPercentAppliesOnlyToDirectSubordinates) {
    // Иерархия: Worker W → Foreman F → Foreman G.
    // G должен получать 7 % ТОЛЬКО от F.own (не от W).
    Factory f;
    Worker* w = f.hireWorker("W", 30'000.0, Date(2022, 1, 1)); // 4 года → +40 %
    Foreman* F = f.hireForeman("F", 50'000.0, Date(2022, 1, 1)); // 4 года → +20 %
    Foreman* G = f.hireForeman("G", 40'000.0, Date(2022, 1, 1)); // 4 года → +20 %

    f.setManager(w, F);
    f.setManager(F, G);

    auto r = f.calculateSalaries(Date(2026, 1, 1));

    const double wSalary = 30'000.0 * 1.4;                        // 42000
    const double FSalary = 50'000.0 * 1.2 + 0.07 * wSalary;       // 62940
    const double GSalary = 40'000.0 * 1.2 + 0.07 * FSalary;       // 52405.8

    EXPECT_NEAR(infoFor(r, w).own, wSalary, kEps);
    EXPECT_NEAR(infoFor(r, F).own, FSalary, kEps);
    EXPECT_NEAR(infoFor(r, G).own, GSalary, kEps);

    // У G descendants = W.own + F.own (+ собственные descendants F, которых нет)
    EXPECT_NEAR(infoFor(r, G).descendants, wSalary + FSalary, kEps);
}

// =============================================================================
//  Manager
// =============================================================================
TEST(Manager, NoSubordinates_ReturnsBaseSalary) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2026, 1, 1));

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, m).own, 100'000.0, kEps);
}

TEST(Manager, ThreePercentOfAllDescendants_IncludingIndirect) {
    // Manager → Worker1
    //         → Foreman → Worker2
    Factory f;
    Worker* w1 = f.hireWorker("W1", 20'000.0, Date(2022, 1, 1)); // +40 % → 28000
    Worker* w2 = f.hireWorker("W2", 30'000.0, Date(2022, 1, 1)); // +40 % → 42000
    Foreman* fm = f.hireForeman("F", 50'000.0, Date(2022, 1, 1)); // +20 %
    Manager* mg = f.hireManager("M", 100'000.0, Date(2022, 1, 1));

    f.setManager(w2, fm);
    f.setManager(w1, mg);
    f.setManager(fm, mg);

    auto r = f.calculateSalaries(Date(2026, 1, 1));

    const double w1Salary = 20'000.0 * 1.4;                            // 28000
    const double w2Salary = 30'000.0 * 1.4;                            // 42000
    const double fmSalary = 50'000.0 * 1.2 + 0.07 * w2Salary;          // 62940
    const double mgSalary = 100'000.0 + 0.03 * (w1Salary + fmSalary + w2Salary);

    EXPECT_NEAR(infoFor(r, mg).own, mgSalary, kEps);
    EXPECT_NEAR(infoFor(r, mg).descendants,
        w1Salary + fmSalary + w2Salary, kEps);
}

TEST(Manager, NoSeniorityBonus_EvenAfterManyYears) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2002, 1, 1)); // 24 года

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    EXPECT_NEAR(infoFor(r, m).own, 100'000.0, kEps);
}

// =============================================================================
//  Hierarchy management
// =============================================================================
TEST(Hierarchy, SetManager_CreatesLink) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2026, 1, 1));
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));

    f.setManager(w, m);

    EXPECT_EQ(w->manager(), m);
    ASSERT_EQ(m->subordinates().size(), 1u);
    EXPECT_EQ(m->subordinates()[0], w);
}

TEST(Hierarchy, SetManager_IdempotentCall) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2026, 1, 1));
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));

    f.setManager(w, m);
    f.setManager(w, m); // повторно

    EXPECT_EQ(m->subordinates().size(), 1u);
}

TEST(Hierarchy, SetManager_ReassignsBetweenManagers) {
    Factory f;
    Manager* m1 = f.hireManager("M1", 100'000.0, Date(2026, 1, 1));
    Manager* m2 = f.hireManager("M2", 100'000.0, Date(2026, 1, 1));
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));

    f.setManager(w, m1);
    f.setManager(w, m2);

    EXPECT_EQ(w->manager(), m2);
    EXPECT_TRUE(m1->subordinates().empty());
    ASSERT_EQ(m2->subordinates().size(), 1u);
}

TEST(Hierarchy, SetManager_ThrowsOnSelfAssignment) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2026, 1, 1));

    EXPECT_THROW(f.setManager(m, m), std::invalid_argument);
}

TEST(Hierarchy, SetManager_ThrowsOnCycle) {
    Factory f;
    Manager* m1 = f.hireManager("M1", 100'000.0, Date(2026, 1, 1));
    Manager* m2 = f.hireManager("M2", 100'000.0, Date(2026, 1, 1));
    Manager* m3 = f.hireManager("M3", 100'000.0, Date(2026, 1, 1));

    f.setManager(m2, m1);   // m1 → m2
    f.setManager(m3, m2);   // m1 → m2 → m3

    // Попытка сделать m1 подчинённым m3 создаст цикл
    EXPECT_THROW(f.setManager(m1, m3), std::invalid_argument);
}

TEST(Hierarchy, SetManager_ThrowsIfManagerIsWorker) {
    Factory f;
    Worker* w1 = f.hireWorker("W1", 30'000.0, Date(2026, 1, 1));
    Worker* w2 = f.hireWorker("W2", 30'000.0, Date(2026, 1, 1));

    // У рабочего не может быть подчинённых
    EXPECT_THROW(f.setManager(w2, w1), std::invalid_argument);
}

TEST(Hierarchy, SetManager_ThrowsOnNullArguments) {
    Factory f;
    Manager* m = f.hireManager("M", 100'000.0, Date(2026, 1, 1));

    EXPECT_THROW(f.setManager(nullptr, m), std::invalid_argument);
    EXPECT_THROW(f.setManager(m, nullptr), std::invalid_argument);
}

// =============================================================================
//  Factory aggregation
// =============================================================================
TEST(Factory, TotalSalary_IsSumOfAllOwnSalaries) {
    Factory f;
    Manager* mg = f.hireManager("M", 100'000.0, Date(2022, 1, 1));
    Foreman* fm = f.hireForeman("F", 50'000.0, Date(2022, 1, 1));
    Worker* w1 = f.hireWorker("W1", 30'000.0, Date(2022, 1, 1));
    Worker* w2 = f.hireWorker("W2", 30'000.0, Date(2024, 1, 1));

    f.setManager(fm, mg);
    f.setManager(w1, fm);
    f.setManager(w2, fm);

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    double expected = 0.0;
    for (const auto& [e, si] : r) expected += si.own;

    EXPECT_NEAR(f.calculateTotalSalary(Date(2026, 1, 1)), expected, kEps);
}

TEST(Factory, EmptyFactory_TotalSalaryIsZero) {
    Factory f;
    EXPECT_NEAR(f.calculateTotalSalary(Date(2026, 1, 1)), 0.0, kEps);
}

TEST(Factory, IndependentRoots_EachCounted) {
    Factory f;
    Manager* m1 = f.hireManager("M1", 100'000.0, Date(2026, 1, 1));
    Manager* m2 = f.hireManager("M2", 100'000.0, Date(2026, 1, 1));
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));

    f.setManager(w, m1); // m2 остаётся без подчинённых

    auto r = f.calculateSalaries(Date(2026, 1, 1));
    // m2 не зависит от m1 / w
    EXPECT_NEAR(infoFor(r, m2).own, 100'000.0, kEps);
    EXPECT_NEAR(infoFor(r, m2).descendants, 0.0, kEps);
}

TEST(Factory, DeepHierarchy_IsHandledWithoutStackOverflow) {
    // 50 000 менеджеров, выстроенных в цепочку. Проверяем,
    // что итеративный обход не переполняет стек и считает корректно.
    Factory f;
    constexpr int kDepth = 50'000;

    Manager* first = f.hireManager("M0", 1'000.0, Date(2026, 1, 1));
    Manager* prev = first;
    for (int i = 1; i < kDepth; ++i) {
        Manager* next = f.hireManager("M" + std::to_string(i),
            1'000.0, Date(2026, 1, 1));
        f.setManager(next, prev);
        prev = next;
    }

    // Не должно падать
    auto total = f.calculateTotalSalary(Date(2026, 1, 1));
    EXPECT_GT(total, 0.0);
}

// =============================================================================
//  Polymorphism / type identification
// =============================================================================
TEST(Polymorphism, TypeNameAndDynamicCast) {
    Factory f;
    Worker* w = f.hireWorker("W", 30'000.0, Date(2026, 1, 1));
    Foreman* F = f.hireForeman("F", 40'000.0, Date(2026, 1, 1));
    Manager* M = f.hireManager("M", 50'000.0, Date(2026, 1, 1));

    EXPECT_EQ(w->typeName(), "Worker");
    EXPECT_EQ(F->typeName(), "Foreman");
    EXPECT_EQ(M->typeName(), "Manager");

    // Полиморфный доступ через базовый указатель
    std::vector<Employee*> all = { w, F, M };
    for (Employee* e : all) {
        EXPECT_FALSE(e->name().empty());
        EXPECT_GT(e->baseSalary(), 0.0);
    }
}

// =============================================================================
//  Integration
// =============================================================================
TEST(Integration, FullScenario_MatchesManualComputation) {
    Factory f;

    Manager* director = f.hireManager("Director", 100'000.0, Date(2017, 1, 1));
    Foreman* foreman1 = f.hireForeman("F1", 60'000.0, Date(2020, 3, 1));
    Foreman* foreman2 = f.hireForeman("F2", 55'000.0, Date(2022, 6, 1));

    Worker* w1 = f.hireWorker("W1", 30'000.0, Date(2023, 4, 1));
    Worker* w2 = f.hireWorker("W2", 32'000.0, Date(2021, 9, 1));
    Worker* w3 = f.hireWorker("W3", 28'000.0, Date(2024, 1, 1));
    Worker* w4 = f.hireWorker("W4", 31'000.0, Date(2019, 11, 1));

    f.setManager(foreman1, director);
    f.setManager(foreman2, director);
    f.setManager(w1, foreman1);
    f.setManager(w2, foreman1);
    f.setManager(w3, foreman2);
    f.setManager(w4, foreman2);

    auto r = f.calculateSalaries(Date(2026, 1, 1));

    // Ожидания (по правилам):
    //   w1: 2021-04 → 2 полных года → 30 000 * 1.2 = 36 000
    //   w2: 2019-09 → 4 полных года → 32 000 * 1.4 = 44 800
    //   w3: 2022-01 → 2 полных года → 28 000 * 1.2 = 33 600
    //   w4: 2017-11 → 6 полных лет → 31 000 * 1.6 = 49 600
    EXPECT_NEAR(infoFor(r, w1).own, 36'000.0, kEps);
    EXPECT_NEAR(infoFor(r, w2).own, 44'800.0, kEps);
    EXPECT_NEAR(infoFor(r, w3).own, 33'600.0, kEps);
    EXPECT_NEAR(infoFor(r, w4).own, 49'600.0, kEps);

    //   F1: 2018-03 → 5 полных лет → 60 000 * 1.25 + 0.07 * (w1+w2)
    const double f1 = 60'000.0 * 1.25 + 0.07 * (36'000.0 + 44'800.0);
    EXPECT_NEAR(infoFor(r, foreman1).own, f1, kEps);

    //   F2: 2020-06 → 3 полных года → 55 000 * 1.15 + 0.07 * (w3+w4)
    const double f2 = 55'000.0 * 1.15 + 0.07 * (33'600.0 + 49'600.0);
    EXPECT_NEAR(infoFor(r, foreman2).own, f2, kEps);

    //   Director: без надбавки за выслугу, 3 % от ВСЕХ потомков
    const double descendants = 36'000.0 + 44'800.0 + 33'600.0 + 49'600.0 + f1 + f2;
    EXPECT_NEAR(infoFor(r, director).own, 100'000.0 + 0.03 * descendants, kEps);

    // Сумма по заводу
    const double expectedTotal = 36'000.0 + 44'800.0 + 33'600.0 + 49'600.0
        + f1 + f2 + infoFor(r, director).own;
    EXPECT_NEAR(f.calculateTotalSalary(Date(2026, 1, 1)), expectedTotal, kEps);
}
