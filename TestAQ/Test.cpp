#include <iostream>

#include "lazy_import.hpp"

int main()
{
    auto handle = LAZY_IMPORT_MODULE("ntdll.dll");

    char buffer[MAX_PATH]{ 0 };
    LAZY_IMPORT(GetModuleFileNameA)(handle, buffer, MAX_PATH);

    std::cout << buffer << '\n';

    std::cin.get();
}
