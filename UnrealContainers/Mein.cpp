#pragma once
#include <Windows.h>
#include <iostream>
#include <format>
#include "Containers.h"
#include "CLASSES_FOR_TESTING.h"

using namespace UE;

void GetSize()
{
    std::cout << sizeof(TArray<int>) << "\n";       // 16 / 16 Working
    std::cout << sizeof(TBitArray) << "\n";         // 32 / 32 Working
    std::cout << sizeof(TSparseArray<int>) << "\n"; // 56 / 56 Not Tested
    //std::cout << sizeof(TSet<int>) << "\n";         // 80 / 80 Not Tested
    //std::cout << sizeof(TMap<int, float>) << "\n";  // 80 / 80 Not Tested
}
struct FText
{
    uint8 pad[0x18];
};

void Main()
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    GObjects = reinterpret_cast<FUObjectArray*>(uintptr_t(GetModuleHandle(0)) + 0x62717C0);

    for (int i = 0; i < GObjects->Num(); i++)
    {
        UObject* current = GObjects->ByIndex(i);

        if (!current)
            continue;


        if (current->GetFullName() == "UserDefinedEnum Engine.Default__UserDefinedEnum")
        {
            std::cout << current << "\n";

            TSet<TPair<FName, FText>>& mySet = *(TSet<TPair<FName, FText>>*)((uint8*)current + 0x60);
            TMap<FName, FText>& myMap = *(TMap<FName, FText>*)((uint8*)current + 0x60);
            TBitArray& myBits = *(TBitArray*)((uint8*)current + 0x70);

            std::cout << "myMap.IsValid(): " << (myMap.IsValid() ? "true\n" : "false\n");

            myMap.Initialize();

            std::cout << "myMap.IsValid(): " << (myMap.IsValid() ? "true\n" : "false\n");
        }
    }

}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
        break;
    }
    case DLL_PROCESS_DETACH: {
        break;
    }
    }
    return TRUE;
}