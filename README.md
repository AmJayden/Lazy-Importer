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
