#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <exception>
#include <limits>

struct TableRow {
    double x;
    double t;
    double u;
};

using Table = std::vector<TableRow>;

Table table_1_1;
Table table_1_00;
Table table_00_1;

bool loadTableFromFile(const std::string& filename, Table& table) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cout << "WARNING: Cannot open file '" << filename << "'" << std::endl;
        return false;
    }
    double x, t, u;
    table.clear();
    while (fin >> x >> t >> u) {
        table.push_back({ x, t, u });
    }
    return true;
}

double interpolate(const Table& table, double x, bool useT) {
    if (table.empty()) return 0;

    if (x <= table.front().x)
        return useT ? table.front().t : table.front().u;

    if (x >= table.back().x)
        return useT ? table.back().t : table.back().u;

    for (size_t i = 1; i < table.size(); ++i) {
        if (x <= table[i].x) {  // <= замість <
            double x0 = table[i - 1].x;
            double x1 = table[i].x;
            double y0 = useT ? table[i - 1].t : table[i - 1].u;
            double y1 = useT ? table[i].t : table[i].u;

            if (x1 != x0) {  // Перевірка ділення на нуль
                return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
            }
            else {
                return y0;
            }
        }
    }
    return 0;
}

//Алгоритм 3
double fun_algorithm3(double x, double y, double z) {
    return 1.3498 * z + 2.2362 * y - 2.348 * x * y;
}

struct FunException : public std::exception {
    double x, y, z;
    std::string message;

    FunException(double x_, double y_, double z_, const std::string& msg = "FunException")
        : x(x_), y(y_), z(z_), message(msg) {
    }

    const char* what() const noexcept override {
        return message.c_str();
    }
};

struct Algorithm1Exception : public FunException {
    Algorithm1Exception(double x, double y, double z)
        : FunException(x, y, z, "Algorithm 1 Exception") {
    }
};

struct Algorithm2Exception : public FunException {
    Algorithm2Exception(double x, double y, double z)
        : FunException(x, y, z, "Algorithm 2 Exception") {
    }
};

struct Algorithm3Exception : public FunException {
    Algorithm3Exception(double x, double y, double z)
        : FunException(x, y, z, "Algorithm 3 Exception") {
    }
};

//Прототипи функцій
double fun_algorithm2(double x, double y, double z);
double Gold1(double x, double y, double z);
double Glr1(double x, double y, double z);

//Функції T і U 
double compute_TU(double x, bool useT, double y, double z) {
    std::string filename;
    double transformedX = x;

    if (std::abs(x) <= 1) {
        filename = "dat_X_1_1.dat";
        transformedX = x;
    }
    else if (x < -1) {
        transformedX = 1.0 / x;
        filename = "dat_X00_1.dat";
    }
    else {  // x > 1
        transformedX = 1.0 / x;
        filename = "dat_X1_00.dat";
    }

    Table* tablePtr = nullptr;
    if (filename == "dat_X_1_1.dat") tablePtr = &table_1_1;
    else if (filename == "dat_X00_1.dat") tablePtr = &table_00_1;
    else if (filename == "dat_X1_00.dat") tablePtr = &table_1_00;

    if (tablePtr->empty()) {
        if (!loadTableFromFile(filename, *tablePtr)) {
            throw Algorithm3Exception(x, y, z);
        }
    }

    return interpolate(*tablePtr, transformedX, useT);
}

double T(double x, double y, double z) {
    return compute_TU(x, true, y, z);
}

double U(double x, double y, double z) {
    return compute_TU(x, false, y, z);
}

//Функція Srz
double Srz(double x, double y, double z) {
    if (x > y) {
        return T(x, y, z) + U(z, y, z) - T(y, y, z);
    }
    else {
        return T(y, y, z) + U(y, y, z) - U(z, y, z);
    }
}


//Функції Gold, Glr (Алгоритм 1)
double Gold(double x, double y, double z) {
    if (x > y && std::abs(y) > 1e-10) {
        return x / y;
    }
    else if (x < y && std::abs(x) > 1e-10) {
        return y / x;
    }
    else {
        std::cout << "DEBUG: Gold() failed - division by zero or invalid condition (x=" << x << ", y=" << y << ")" << std::endl;
        throw Algorithm1Exception(x, y, z);
    }
}

double Glr(double x, double y, double z) {
    if (std::abs(x) < 1) {
        return x;
    }
    else if (std::abs(x) >= 1 && std::abs(y) < 1) {
        return y;
    }
    else if (std::abs(x) >= 1 && std::abs(y) >= 1) {
        double radicand = x * x + y * y - 4;
        if (radicand > 0.1) {
            return y / std::sqrt(radicand);
        }
        else {
            std::cout << "DEBUG: Glr() failed - radicand too small (" << radicand << ")" << std::endl;
            throw Algorithm1Exception(x, y, z);
        }
    }
    return 0;
}

//Функція Grs (Алгоритм 1) 
double Grs(double x, double y, double z) {
    double term1 = 0.1389 * Srz(x + y, Gold(x, y, z), Glr(x, x * y, z));
    double term2 = 1.8389 * Srz(x - y, Gold(y, x / 5.0, z), Glr(5.0 * x, x * y, z));
    double term3 = 0.83 * Srz(x - 0.9, Glr(y, x / 5.0, z), Gold(5.0 * y, y, z));
    return term1 + term2 + term3;
}

//Функція fun (Алгоритм 1)
double fun(double x, double y, double z) {
    double term1 = x * x * Grs(y, z, z);
    double term2 = y * y * Grs(x, z, z);
    double term3 = 0.33 * x * y * Grs(x, z, z);
    return term1 + term2 + term3;
}

//Функції Gold1, Glr1, Grs1 (Алгоритм 2)
double Gold1(double x, double y, double z) {
    if (x > y && std::abs(y) > 0.1) {
        return x / y;
    }
    else if (x <= y && std::abs(x) > 0.1) {
        return y / x;
    }
    else if (x < y && std::abs(x) <= 0.1) {
        return 0.15;
    }
    else if (std::abs(y) <= 1e-10) {
        return 0.1;
    }
    return 0;
}

double Glr1(double x, double y, double z) {
    if (std::abs(x) < 1) {
        return x;
    }
    else {
        return y;
    }
}

double Grs1(double x, double y) {
    double term1 = 0.14 * Srz(x + y, Gold1(x, y, 0), Glr1(x, x * y, 0));
    double term2 = 1.83 * Srz(x - y, Gold1(y, x / 5.0, 0), Glr1(4.0 * x, x * y, 0));
    double term3 = 0.83 * Srz(x, Glr1(y, x / 4.0, 0), Gold1(4.0 * y, y, 0));
    return term1 + term2 + term3;
}

//Функція fun_algorithm2 (Алгоритм 2)
double fun_algorithm2(double x, double y, double z) {
    return x * Grs1(x, y) + y * Grs1(y, z) + z * Grs1(z, x);
}

int main() {
    std::cin.exceptions(std::ios::failbit | std::ios::badbit);

    double x, y, z;

    try {
        std::cout << "Enter values x, y, z: ";
        std::cin >> x >> y >> z;
        std::cout << std::endl;


        std::cout << "=== Trying Algorithm 1 ===" << std::endl;
        double result = fun(x, y, z);
        std::cout << "SUCCESS: Algorithm 1 completed successfully!" << std::endl;
        std::cout << "Result fun(x,y,z) = " << result << std::endl;
    }
    catch (const std::ios_base::failure& e) {
        std::cerr << "Input error: invalid numeric input. Please enter valid numbers." << std::endl;
        // Очищуємо помилковий стан і очищаємо буфер вводу
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return 1;
    }
    catch (const Algorithm1Exception& e1) {
        std::cout << "Algorithm 1 failed, switching to Algorithm 2..." << std::endl;
        try {
            double result2 = fun_algorithm2(e1.x, e1.y, e1.z);
            std::cout << "SUCCESS: Algorithm 2 completed successfully!" << std::endl;
            std::cout << "Algorithm 2 result = " << result2 << std::endl;
        }
        catch (const Algorithm2Exception& e2) {
            std::cout << "Algorithm 2 also failed, switching to Algorithm 3..." << std::endl;
            double result3 = fun_algorithm3(e2.x, e2.y, e2.z);
            std::cout << "SUCCESS: Algorithm 3 completed successfully!" << std::endl;
            std::cout << "Algorithm 3 result = " << result3 << std::endl;
        }
        catch (const Algorithm3Exception& e3) {
            std::cout << "Algorithm 2 failed due to file loading error, switching to Algorithm 3..." << std::endl;
            double result3 = fun_algorithm3(e3.x, e3.y, e3.z);
            std::cout << "SUCCESS: Algorithm 3 completed successfully!" << std::endl;
            std::cout << "Algorithm 3 result = " << result3 << std::endl;
        }
    }
    catch (const Algorithm3Exception& e3) {
        std::cout << "Algorithm 1 failed due to file loading error, switching to Algorithm 3..." << std::endl;
        double result3 = fun_algorithm3(e3.x, e3.y, e3.z);
        std::cout << "SUCCESS: Algorithm 3 completed successfully!" << std::endl;
        std::cout << "Algorithm 3 result = " << result3 << std::endl;
    }
    catch (const FunException& e) {
        std::cerr << "FunException caught: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "ERROR: Unexpected error occurred.\n";
    }

    return 0;
}
