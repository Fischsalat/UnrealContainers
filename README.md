# UnrealContainers
### UnrealEngine typs with iterators

## Supported Types:
- TArray
- FString
- TSet
- TMap
- TSparseArray
- FBitARray

## Containers.h
First implementation, may not compile. \
Requires offsets for FMemory::Malloc, FMemory::Realloc, FMemory::Free.
### Usage:
```c++
FMemory_Malloc = reinterpret_cast<decltype(FMemory_Malloc)>(MallocAddress);
FMemory_Realloc = reinterpret_cast<decltype(FMemory_Realloc)>(ReallocAddress);
FMemory_Free = reinterpret_cast<decltype(FMemory_Free)>(FreeAddress);

TArray<int> MyArray(0x10); // allocates 0x10 bytes with FMemory::Malloc

TMap<int, void*>& SomeMap = GetSomeMap();
for (TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## ContainersRewrite.h
Second implementation, improved stability. \
Requires only the offset of FMemory::Realloc.
### Usage:
```c++
FMemory::Init(ReallocAddress);

TArray<int> MyArray(0x10); // Allocates 0x10 bytes with FMemory::Malloc (freed on destruction)

TMap<int, void*>& SomeMap = GetSomeMap();
for (TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## UnrealContainers.h
Third implementation, has a few extra fixes and better constructors/destructors, as well as move-/copy-constructors and assignment operators. \
Requires only the offset of FMemory::Realloc.
### Usage:
```c++
FMemory::Init(ReallocAddress);

TArray<int> MyArray(0x10); // Allocates 0x10 bytes with FMemory::Malloc (freed on destruction)
TArray<int> Other = MyArray; // Copies MyArray
TArraay<int> Another = std::move(MyArray); // Moves data from MyArray, leaving it empty
MyArray = Other; // Copies Other back into MyArray
Other.Add(40); // Adds to the array

TMap<int, void*> SomeMap = GetSomeMap(); // Copies map (using TMap<..>& is still advised)
for (TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```

## UnrealContainersNoAlloc.h
This is the third implementation, but without any allocating functionality (except c-style allocations from the standard-library). \
Arrays or strings allocated by UnrealEngine **will not** be freed on destruction of the container. Be careful to avoid leaking too much memory. \
Requires no offsets at all. \
This implementation exists because calling FMemory::Realloc sometimes caused crashes.

### Usage:
```c++
TAllocatedArray<int> MyArray(0x10); // Allocates 0x10 bytes with malloc from the c-stdlib (freed on destruction)
TArray<int> Other = GetSomeArray(); // NOT freed on destruction, no FMemory function to free the block
TArraay<int> Another = Other; // Does a "shallow" copy, now both Other and Another share the same buffer (bad)
TArraay<int> AnotherOne = std::move(MyArray);  // Does a "shallow" copy, now both Other, Another and AnotherOne share the same buffer (bad)

TMap<int, void*>& SomeMap = GetSomeMap();
for (TPair<int, void*>& Pair : SomeMap)
    std::cout << Pair.second << std::endl;
```
