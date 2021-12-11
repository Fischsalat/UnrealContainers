#pragma once
#include <Windows.h>
#include <iostream>
#include <format>
#include "Containers.h"
#include "CLASSES_FOR_TESTING.h"

using namespace UE;

struct FText
{
	uint8 pad[0x18];
};

#define Super(obj) *(UObject**)(int64(obj) + 0x30)

inline bool IsA(UObject* Obj, UObject* Class)
{
	if (!Obj || Obj->GetFullName().find("Default__") != -1)
		return false;
	
	for (UObject* clss = Obj->privateClass; clss; clss = Super(clss))
	{
		if (clss == Class)
			return true;
	}
	return false;
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
		
		if (current->GetFullName() == "UserDefinedEnum Engine.Default__UserDefinedEnum")
		{
			std::cout << current << "\n";

			TSet<TPair<FName, FText>>& mySet = *(TSet<TPair<FName, FText>>*)((uint8*)current + 0x60);
			TMap<FName, FText>& myMap = *(TMap<FName, FText>*)((uint8*)current + 0x60);
			TBitArray& myBits = *(TBitArray*)((uint8*)current + 0x70);
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