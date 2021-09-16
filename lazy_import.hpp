#pragma once
#include <cstdint>
#include <cstddef>
#include <Windows.h>
#include <winternl.h>

#pragma warning(disable : 26812)

#define LAZY_IMPORT_MOD(dll, func) (reinterpret_cast<decltype(&func)>(lazy_importer::find_export_mod<dll, #func>()))
#define LAZY_IMPORT(func) (reinterpret_cast<decltype(&func)>(lazy_importer::find_first_export<#func>()))
#define LAZY_IMPORT_MODULE(mod) (reinterpret_cast<HMODULE>(lazy_importer::get_module_handle(lazy_importer::wrapper_constant_v<lazy_importer::hash::hash_str(mod)>)))

#ifdef _DEBUG
#define LAZY_IMPORT_INLINE 
#else
#define LAZY_IMPORT_INLINE __forceinline
#endif

namespace lazy_importer
{
    // wrap constant values in an enum, so we can ENSURE
    // they're generated at compile time
    template <auto v>
    struct wrapper_constant
    {
        enum : decltype(v)
        {
            value = v
        };
    };

    template <auto v>
    inline constexpr auto wrapper_constant_v = wrapper_constant<v>::value;

    namespace detail
    {

        struct PEB_LDR_DATA
        {
            unsigned long dummy_0;
            unsigned long dummy_1;
            const char* dummy_2;
            LIST_ENTRY*  in_load_order_module_list;
        };

        struct LDR_DATA_TABLE_ENTRY32
        {
                LIST_ENTRY InLoadOrderLinks;

                std::uint8_t pad[16];
                std::uintptr_t dll_base;
                std::uintptr_t entry_point;
                std::size_t size_of_image;

                UNICODE_STRING full_name;
                UNICODE_STRING base_name;
        };

        struct LDR_DATA_TABLE_ENTRY64
        {
            LIST_ENTRY InLoadOrderLinks;
            LIST_ENTRY dummy_0;
            LIST_ENTRY dummy_1;

            std::uintptr_t dll_base;
            std::uintptr_t entry_point;
            union {
                unsigned long size_of_image;
                const char* _dummy;
            };
            
            UNICODE_STRING full_name;
            UNICODE_STRING base_name;
        };

        // x64 and x86 versions vary
#ifdef _M_X64
        using LDR_DATA_TABLE_ENTRY = LDR_DATA_TABLE_ENTRY64;
#else
        using LDR_DATA_TABLE_ENTRY = LDR_DATA_TABLE_ENTRY32;
#endif
    }

    // compile time wrapper struct to pass strings through templates
    template <auto sz>
    struct comp_string
    {
        constexpr comp_string(const char(&str)[sz])
        {
            std::copy_n(str, sz, value);
        }

        char value[sz];
    };

    // fnv-1a hashing strings
    namespace hash
    {
        inline constexpr auto offset = 0xcbf29ce484222325ull;
        inline constexpr auto prime = 1099511628211ull;

        constexpr std::uint64_t hash_str(const char* const str)
        {
            // dumb method of obtaining size
            auto sz = 0u;
            for (; str[sz] >= 0x20 && str[sz] <= 0x7E; ++sz) {}

            auto hash = offset;
            for (auto i = 0u; i < sz; ++i)
            {
                const auto c = str[i];

                // case insensitive
                hash ^= (c >= 'A' && c <= 'Z' ? (c | (1 << 5)) : c);
                hash *= prime;
            }

            return hash;
        }
    }

    namespace util
    {
        // basic conversion function using no calls and checking ascii range
        __forceinline void wide_to_ascii(char* const buffer, const wchar_t* const wide_char)
        {
            for (auto i = 0u; wide_char[i] >= 0x20 && wide_char[i] <= 0x7E; ++i)
                buffer[i] = static_cast<char>(wide_char[i]);
        }
    }

    __forceinline detail::PEB_LDR_DATA* get_peb_ldr()
    {
#ifdef _M_IX86
        return reinterpret_cast<detail::PEB_LDR_DATA*>(reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<std::uintptr_t>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock->Ldr);
#else
        return reinterpret_cast<detail::PEB_LDR_DATA*>(reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<std::uintptr_t>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock->Ldr);
#endif
    }

    __forceinline std::uint8_t* get_module_handle(const std::uint64_t dll_hash)
    {
        auto peb_ldr_data = get_peb_ldr();

        auto module = peb_ldr_data->in_load_order_module_list->Flink;

        const auto last_module = peb_ldr_data->in_load_order_module_list->Blink;

        // result
        std::uint8_t* module_handle{};

        // loop through all modules in load order
        while (true)
        {
            auto module_entry = reinterpret_cast<detail::LDR_DATA_TABLE_ENTRY*>(module);

            if (module_entry->dll_base)
            {
                const auto module_wide_name = module_entry->base_name.Buffer;

                char module_name[256];
                util::wide_to_ascii(module_name, module_wide_name);

                if (dll_hash == hash::hash_str(module_name))
                {
                    module_handle = reinterpret_cast<std::uint8_t*>(module_entry->dll_base);

                    break;
                }
            }

            if (module == last_module)
                break;

            // next module in list
            module = module->Flink;
        }

        // return result (if set)
        return module_handle;
    }

    // find an export with hash primitives instead of strings
    __forceinline std::uintptr_t primitive_find_export(std::uint64_t dll_hash, std::uint64_t function_hash)
    {
        auto module_handle = get_module_handle(dll_hash);

        if (!module_handle)
            return {};
        
        const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);

        const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(module_handle + dos_header->e_lfanew);

        const auto export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(module_handle + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

        const auto functions = reinterpret_cast<std::uint32_t*>(module_handle + export_directory->AddressOfFunctions);
        const auto names = reinterpret_cast<std::uint32_t*>(module_handle + export_directory->AddressOfNames);
        const auto ordinals = reinterpret_cast<std::uint16_t*>(module_handle + export_directory->AddressOfNameOrdinals);

        for (auto i = 0u; i < export_directory->NumberOfFunctions; ++i)
        {
            if (function_hash == hash::hash_str(reinterpret_cast<const char*>(module_handle) + names[i]))
            {
                return reinterpret_cast<std::uintptr_t>(module_handle + functions[ordinals[i]]);
            }
        }

        return {};
    }

    // find export with string literals
    template <comp_string dll_name, comp_string function_name>
    __forceinline std::uintptr_t find_export_mod()
    {
        constexpr auto dll_hash = wrapper_constant_v<hash::hash_str(dll_name.value)>;
        constexpr auto function_hash = wrapper_constant_v<hash::hash_str(function_name.value)>;

        return primitive_find_export(dll_hash, function_hash);
    }

    // find first export matching `function_name` literal under all modules
    template <comp_string function_name>
    __forceinline std::uintptr_t find_first_export()
    {
        constexpr auto function_hash = wrapper_constant_v<hash::hash_str(function_name.value)>;

        auto peb_ldr_data = get_peb_ldr();

        auto module = peb_ldr_data->in_load_order_module_list->Flink;

        const auto last_module = peb_ldr_data->in_load_order_module_list->Blink;

        std::uint8_t* module_handle{};

        while (true)
        {
            auto module_entry = reinterpret_cast<detail::LDR_DATA_TABLE_ENTRY*>(module);

            if (module_entry->dll_base)
            {
                const auto module_wide_name = module_entry->base_name.Buffer;

                char module_name[256];
                util::wide_to_ascii(module_name, module_wide_name);

                if (auto x = primitive_find_export(hash::hash_str(module_name), function_hash))
                    return x;

            }

            if (module == last_module)
                break;

            module = module->Flink;
        }

        return {};
    }
}
