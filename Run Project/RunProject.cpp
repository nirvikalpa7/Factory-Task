
#include <iomanip>
#include <iostream>

#include "../Source/factory.h"

// ============================================================
//  Factory Demo
// ============================================================

int main() {
    Factory factory;

    Manager* director = factory.hireManager("Ivanov", 100000.0, Date(2023, 1, 1));
    Foreman* foreman1 = factory.hireForeman("Petrov", 60000.0, Date(2019, 3, 1));
    Foreman* foreman2 = factory.hireForeman("Sidorov", 55000.0, Date(2022, 6, 1));

    Worker* worker1 = factory.hireWorker("Smirnov", 30000.0, Date(2021, 4, 1));
    Worker* worker2 = factory.hireWorker("Kuznetsov", 32000.0, Date(2019, 9, 1));
    Worker* worker3 = factory.hireWorker("Popov", 28000.0, Date(2022, 1, 1));
    Worker* worker4 = factory.hireWorker("Vasiliev", 31000.0, Date(2024, 11, 1));

    factory.setManager(foreman1, director);
    factory.setManager(foreman2, director);
    factory.setManager(worker1, foreman1);
    factory.setManager(worker2, foreman1);
    factory.setManager(worker3, foreman2);
    factory.setManager(worker4, foreman2);

    const Date month(2026, 10, 1);
    auto results = factory.calculateSalaries(month);

    std::cout << std::endl << "Payroll for " << month << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::left << std::setw(10) << "Type"
        << std::left << std::setw(12) << "Name"
        << std::right << std::setw(12) << "Base"
        << std::right << std::setw(14) << "Salary" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto& emp : factory.employees()) {
        std::cout << std::left << std::setw(10) << emp->typeName()
            << std::left << std::setw(12) << emp->name()
            << std::right << std::fixed << std::setprecision(2)
            << std::setw(12) << emp->baseSalary()
            << std::setw(14) << results.at(emp.get()).own << std::endl;
    }
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Total factory payroll: "
        << std::fixed << std::setprecision(2)
        << factory.calculateTotalSalary(month) << std::endl;

    return 0;
}
