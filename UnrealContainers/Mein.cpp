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


        if (current->GetFullName() == "FortUIDataConfiguration FortniteUI.Default__FortUIDataConfiguration")
        {
            std::cout << current << "\n";

            TSparseArray<TPair<int32, int32>>& myArray = *(TSparseArray<TPair<int32, int32>>*)((uint8*)current + 0x138);
            TSet<TPair<int32, int32>>& mySet = *(TSet<TPair<int32, int32>>*)((uint8*)current + 0x138);
            TMap<int32, int32>& myMap = *(TMap<int32, int32>*)((uint8*)current + 0x138);
            TBitArray& myBits = *(TBitArray*)((uint8*)current + 0x148);

            std::cout << "\n";
            
            int unfunnytestint = 0;

            std::cout << "\nTBitAray:\n";
            for (auto element : myBits)
            {
                std::cout << (element ? "True\n" : "False\n");
            }

            std::cout << "\nTSparseArray:\n";
            for (auto element : myArray)
            {
                std::cout << element.Key() << "\t" << element.Value() << "\n";
            }

            std::cout << "\nTSet:\n";
            for (auto element : mySet)
            {
                std::cout << element.Value.Key() << "\t" << element.Value.Value() << "\n";
            }

            std::cout << "\nTMap:\n";
            for (auto element : myMap)
            {
                std::cout << element.Value.Key() << "\t" << element.Value.Value() << "\n";
            }

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