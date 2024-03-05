# UnrealContainers
### UnrealEngine typs with iterators (use UnrealContainers.h or UnrealContainersNoAlloc.h)

## Supported Types:
- TArray
- FString
- TSet
- TMap
- TSparseArray
- FBitArray

## Containers.h
First implementation, may not compile. \
Requires offsets for FMemory::Malloc, FMemory::Realloc, FMemory::Free.
### Usage:
```c++
FMemory_Malloc = reinterpret_cast<decltype(FMemory_Malloc)>(MallocAddress);
FMemory_Realloc = reinterpret_cast<decltype(FMemory_Realloc)>(ReallocAddress);
FMemory_Free = reinterpret_cast<decltype(FMemory_Free)>(FreeAddress);

UE::TArray<int> MyArray(0x10); // allocates 0x10 bytes with FMemory::Malloc

UE::TMap<int, void*>& SomeMap = GetSomeMap();
for (UE::TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## ContainersRewrite.h
Second implementation, improved stability. \
Requires only the offset of FMemory::Realloc.
### Usage:
```c++
UC::FMemory::Init(ReallocAddress);

UC::TArray<int> MyArray(0x10); // Allocates 0x10 bytes with FMemory::Malloc (freed on destruction)

UC::TMap<int, void*>& SomeMap = GetSomeMap();
for (UC::TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## UnrealContainers.h
Third implementation, has a few extra fixes and better constructors/destructors, as well as move-/copy-constructors and assignment operators. \
Requires only the offset of FMemory::Realloc.
### Usage:
```c++
UC::FMemory::Init(ReallocAddress);

UC::TArray<int> MyArray(0x10); // Allocates 0x10 bytes with FMemory::Malloc (freed on destruction)
UC::TArray<int> Other = MyArray; // Copies MyArray
UC::TArraay<int> Another = std::move(MyArray); // Moves data from MyArray, leaving it empty
MyArray = Other; // Copies Other back into MyArray
Other.Add(40); // Adds to the array

UC::TMap<int, void*> SomeMap = GetSomeMap(); // Copies map (using TMap<..>& is still advised)
for (UC::TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## UnrealContainersNoAlloc.h (used by [Dumper-7](https://github.com/Encryqed/Dumper-7))
This is the third implementation, but without any allocating functionality (except c-style allocations from the standard-library). \
Arrays or strings allocated by UnrealEngine **will not** be freed on destruction of the container. Be careful to avoid leaking too much memory. \
Requires no offsets at all. \
This implementation exists because calling FMemory::Realloc sometimes caused crashes.

### Usage:
```c++
UC::TAllocatedArray<int> MyArray(0x10); // Allocates 0x10 bytes with malloc from the c-stdlib (freed on destruction)
UC::TArray<int> Other = GetSomeArray(); // NOT freed on destruction, no FMemory function to free the block
UC::TArraay<int> Another = Other; // Does a "shallow" copy, now both Other and Another share the same buffer (bad)
UC::TArraay<int> AnotherOne = std::move(MyArray);  // Does a "shallow" copy, now both Other, Another and AnotherOne share the same buffer (bad)

UC::TMap<int, void*>& SomeMap = GetSomeMap();
for (UC::TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```
