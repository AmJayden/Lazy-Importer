#include <iostream>
#include <string>
#include <chrono>
#include <Windows.h>
#include <tlhelp32.h>

#include "lazy_import.hpp"

std::uint32_t get_pid(const std::wstring& str)
{
    auto snapshot = LAZY_IMPORT(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!LAZY_IMPORT(Process32FirstW)(snapshot, &pe))
        return LAZY_IMPORT(CloseHandle)(snapshot), 0;

    do
    {
        if (std::wstring(pe.szExeFile).find(str) != std::wstring::npos)
            return LAZY_IMPORT(CloseHandle)(snapshot), pe.th32ProcessID;
    } while (LAZY_IMPORT(Process32NextW)(snapshot, &pe));

    LAZY_IMPORT(CloseHandle)(snapshot);
    return 0;
}

__forceinline std::uint64_t get_tick_count()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main()
{
    auto start_1 = get_tick_count();
    auto handle = LAZY_IMPORT_MODULE("ntdll.dll");

    char buffer[MAX_PATH]{ 0 };
    LAZY_IMPORT(GetModuleFileNameA)(handle, buffer, MAX_PATH);
    
    std::cout << get_tick_count() - start_1 << "ms\n";


    std::cout << buffer << '\n';

    std::wstring input;
    std::getline(std::wcin, input);

    auto start_2 = get_tick_count();
    std::cout << get_pid(input) << '\n';
    std::cout << get_tick_count() - start_2 << "ms\n";


    std::cin.get();
}
