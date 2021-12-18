#pragma once
#include <Windows.h>
#include <iostream>
#include <format>
#include "Containers.h"
#include "CLASSES_FOR_TESTING.h"

using namespace UE;

#define PRINTBOOL(b) std::cout << (b ? "true\n" : "false\n")

struct FText
{
	uint8 pad[0x18];
};

#define SUPER(obj) *(UObject**)(int64(obj) + 0x30)

inline bool IsA(UObject* Obj, UObject* Class)
{
	if (!Obj || Obj->GetName().find("Default__") != -1)
		return false;
	
	for (UObject* clss = Obj->privateClass; clss; clss = SUPER(clss))
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