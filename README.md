# Lazy-Importer
minimalistic msvc-windows exclusive lazy importer for C++

## Credits
* 0x90 ([@AmJayden](https://github.com/AmJayden))
* [@gogo9211](https://github.com/gogo9211)

## What is this?
This lazy importer allows you to dynamically retrieve imported functions and modules.

By hashing the name of the function we want to import, and the module it holds on compile time, we can loop through all modules and compare them, then loop through their exports to retrieve the import.

## What're the benefits?
When an import is retrieved on compile time, there's a call to the pointer in `.idata`. 

A reverse engineer can use this to their advantage, cross referencing it back to your function, and seeing how you call your import.

When lazy importing, your import information is hashed on compile time, then compared with hashes generated on runtime.
Allowing you to get a pointer to your import, but on runtime instead of compile time. 

In order for a reverse engineer to see your lazily imported calls, they must debug and step through to your call.

When combined with anti debug or anti dynamic reverse engineering techniques, it can be very difficult to resolve your imported function.

## Usage
In the case of failure, all functions will return `0` to substitute.

### Lazily importing by module
To lazily import a function with a specific module in mind, do the following.
```cpp
auto f = LAZY_IMPORT_MOD("Kernel32.dll", CloseHandle);
```
With the first argument being a string containing the case insensitive name of your module.

The second argument may be a function, who's hash will be searched for throughout that module.


### Lazily importing regardless of module
In order to lazily import a function, regardless of which module it's in, do the following.
```cpp
auto f = LAZY_IMPORT(CloseHandle);
```
The code above will scan for `CloseHandle`'s hash throughout all present modules, and use the first match.


### Lazily importing a module handle
To get the handle of a module lazily, do the following.
```cpp
auto handle = LAZY_IMPORT_MODULE("ntdll.dll");
```
The first argument passed to `LAZY_IMPORT_MODULE` will be hashed and scanned throughout the present modules.

The first occurence of the hash will be used, and it's handle will be returned (`HMODULE`)
