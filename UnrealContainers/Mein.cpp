#pragma once
#include <Windows.h>
#include <iostream>
#include "Containers.h"

using namespace UE;

void GetSize()
{
    std::cout << sizeof(TBitArray) << "\n";         // 32 / 32
    std::cout << sizeof(TArray<int>) << "\n";       // 16 / 16
    std::cout << sizeof(TSparseArray<int>) << "\n"; // 56 / 56
    std::cout << sizeof(TSet<int>) << "\n";         // 80 / 80
    std::cout << sizeof(TMap<int, float>) << "\n";  // 80 / 80
}

void Main()
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);


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