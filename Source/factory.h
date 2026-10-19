#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stack>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <iomanip>

// ============================================================
//  Date
// ============================================================
struct Date {
    int year{ 0 };
    int month{ 0 }; // 1..12
    int day{ 0 };   // 1..31

    Date(int y, int m, int d) : year(y), month(m), day(d) {}

    friend std::ostream& operator<<(std::ostream& os, const Date& d) {
        os << std::setfill('0')
            << std::setw(4) << d.year << '-'
            << std::setw(2) << d.month << '-'
            << std::setw(2) << d.day
            << std::setfill(' '); // Set default value
        return os;
    }
};

// ============================================================
//  SalaryInfo
// ============================================================
struct SalaryInfo {
    double own{ 0.0 };         // Полная зарплата сотрудника за месяц
    double descendants{ 0.0 }; // Сумма зарплат всех его потомков
};

// ============================================================
//  Employee (abstract base)
// ============================================================
class Employee {
public:
    Employee(std::string name, double baseSalary, Date hireDate)
        : name_(std::move(name)),
        baseSalary_(baseSalary),
        hireDate_(hireDate) {
    }

    virtual ~Employee() = default;
    Employee(const Employee&) = delete;
    Employee& operator=(const Employee&) = delete;

    // Getters
    const std::string& name() const { return name_; }
    double             baseSalary() const { return baseSalary_; }
    const Date& hireDate() const { return hireDate_; }

    Employee* manager() const { return manager_; }
    const std::vector<Employee*>& subordinates() const { return subordinates_; }

    // Structural member functions
    void setManager(Employee* mgr) { manager_ = mgr; }
    void addSubordinate(Employee* sub) { subordinates_.push_back(sub); }
    void removeSubordinate(Employee* s) {
        subordinates_.erase(
            std::remove(subordinates_.begin(), subordinates_.end(), s),
            subordinates_.end());
    }

    // Helpers
    int fullYearsWorked(const Date& month) const {
        int years = month.year - hireDate_.year;
        if (month.month < hireDate_.month) --years;
        return std::max(0, years);
    }

    virtual bool canHaveSubordinates() const { return true; }

    // Polymorphic API
    virtual SalaryInfo computeSalary(
        const Date& month,
        const std::unordered_map<const Employee*, SalaryInfo>& results) const = 0;

    virtual std::string typeName() const = 0;

protected:
    std::string name_;
    double      baseSalary_{ 0.0 };
    Date        hireDate_;
    Employee* manager_{ nullptr };
    std::vector<Employee*> subordinates_;
};

// ============================================================
//  Worker
// ============================================================
class Worker : public Employee {
public:
    using Employee::Employee;

    bool canHaveSubordinates() const override { return false; }

    SalaryInfo computeSalary(
        const Date& month,
        const std::unordered_map<const Employee*, SalaryInfo>&) const override
    {
        const int    years = fullYearsWorked(month);
        const double bonusPercent = std::min(years * 10.0, 100.0);

        SalaryInfo info;
        info.own = baseSalary_ * (1.0 + bonusPercent / 100.0);
        info.descendants = 0.0;
        return info;
    }

    std::string typeName() const override { return "Worker"; }
};

// ============================================================
//  Foreman
// ============================================================
class Foreman : public Employee {
public:
    using Employee::Employee;

    SalaryInfo computeSalary(
        const Date& month,
        const std::unordered_map<const Employee*, SalaryInfo>& results) const override
    {
        const int    years = fullYearsWorked(month);
        const double bonusPercent = std::min(years * 5.0, 40.0);

        double directSubSum = 0.0; // сумма зарплат только ПРЯМЫХ подчинённых
        double allDescSum = 0.0; // сумма зарплат ВСЕХ потомков

        for (const Employee* sub : subordinates_) {
            const SalaryInfo& si = results.at(sub);
            directSubSum += si.own;
            allDescSum += si.own + si.descendants;
        }

        SalaryInfo info;
        info.own = baseSalary_ * (1.0 + bonusPercent / 100.0)
            + 0.07 * directSubSum;
        info.descendants = allDescSum;
        return info;
    }

    std::string typeName() const override { return "Foreman"; }
};

// ============================================================
//  Manager
// ============================================================
class Manager : public Employee {
public:
    using Employee::Employee;

    SalaryInfo computeSalary(
        const Date& /*month*/,
        const std::unordered_map<const Employee*, SalaryInfo>& results) const override
    {
        // У менеджера нет надбавки за выслугу — только 3% от всех потомков.
        double allDescSum = 0.0;
        for (const Employee* sub : subordinates_) {
            const SalaryInfo& si = results.at(sub);
            allDescSum += si.own + si.descendants;
        }

        SalaryInfo info;
        info.own = baseSalary_ + 0.03 * allDescSum;
        info.descendants = allDescSum;
        return info;
    }

    std::string typeName() const override { return "Manager"; }
};

// ============================================================
//  Factory
// ============================================================
class Factory {
public:
    // Hiring
    Worker* hireWorker(const std::string& name, double base, const Date& d) {
        return registerEmployee<Worker>(name, base, d);
    }
    Foreman* hireForeman(const std::string& name, double base, const Date& d) {
        return registerEmployee<Foreman>(name, base, d);
    }
    Manager* hireManager(const std::string& name, double base, const Date& d) {
        return registerEmployee<Manager>(name, base, d);
    }

    // Hierarchy
    void setManager(Employee* subordinate, Employee* manager) {
        if (!subordinate || !manager)
            throw std::invalid_argument("subordinate and manager must be non-null");

        if (!manager->canHaveSubordinates())
            throw std::invalid_argument(
                manager->typeName() + " '" + manager->name() +
                "' cannot have subordinates");

        if (subordinate == manager || isDescendant(subordinate, manager))
            throw std::invalid_argument("cannot create a cycle in the hierarchy");

        if (subordinate->manager() == manager) return; // уже так

        if (subordinate->manager())
            subordinate->manager()->removeSubordinate(subordinate);
        else
            removeRoot(subordinate);

        subordinate->setManager(manager);
        manager->addSubordinate(subordinate);
    }

    // Payroll
    std::unordered_map<const Employee*, SalaryInfo>
        calculateSalaries(const Date& month) const {
        std::vector<Employee*> order = buildPostOrder();

        std::unordered_map<const Employee*, SalaryInfo> results;
        results.reserve(order.size());

        for (Employee* emp : order) {
            SalaryInfo info = emp->computeSalary(month, results);
            results.emplace(emp, info);
        }
        return results;
    }

    double calculateTotalSalary(const Date& month) const {
        auto results = calculateSalaries(month);
        double total = 0.0;
        for (const auto& [emp, info] : results) total += info.own;
        return total;
    }

    // Introspection
    const std::vector<std::unique_ptr<Employee>>& employees() const {
        return employees_;
    }

private:
    template <typename T, typename... Args>
    T* registerEmployee(Args&&... args) {
        static_assert(std::is_base_of_v<Employee, T>,
            "T must derive from Employee");
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();
        employees_.push_back(std::move(ptr));
        roots_.push_back(raw);
        return raw;
    }

    void removeRoot(Employee* emp) {
        roots_.erase(std::remove(roots_.begin(), roots_.end(), emp), roots_.end());
    }

    static bool isDescendant(const Employee* ancestor, const Employee* node) {
        std::stack<const Employee*> stk;
        for (auto* sub : ancestor->subordinates()) stk.push(sub);
        while (!stk.empty()) {
            const Employee* e = stk.top(); stk.pop();
            if (e == node) return true;
            for (auto* s : e->subordinates()) stk.push(s);
        }
        return false;
    }

    std::vector<Employee*> buildPostOrder() const {
        std::vector<Employee*> order;
        order.reserve(employees_.size());

        std::stack<std::pair<Employee*, bool>> stk;
        for (Employee* root : roots_) stk.push({ root, false });

        while (!stk.empty()) {
            auto [emp, processed] = stk.top();
            stk.pop();
            if (processed) {
                order.push_back(emp);
            }
            else {
                stk.push({ emp, true });
                for (Employee* sub : emp->subordinates())
                    stk.push({ sub, false });
            }
        }
        return order;
    }

    std::vector<std::unique_ptr<Employee>> employees_;
    std::vector<Employee*>                 roots_;
};


