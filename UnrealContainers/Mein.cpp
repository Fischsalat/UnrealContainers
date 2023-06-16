#pragma once
#include <Windows.h>
#include <iostream>
#include <format>
//#include "Containers.h"

//#include "ContainersRewrite.h"
#include "ContainersRewrite.h"

//using namespace UE;


#define PRINTBOOL(b) std::cout << (b ? "true\n" : "false\n")

//struct FText
//{
//	uint8 pad[0x18];
//};
//
//#define SUPER(obj) *(UObject**)(int64(obj) + 0x30)
//
//inline bool IsA(UObject* Obj, UObject* Class)
//{
//	if (!Obj || Obj->GetName().find("Default__") != -1)
//		return false;
//	
//	for (UObject* clss = Obj->privateClass; clss; clss = SUPER(clss))
//	{
//		if (clss == Class)
//			return true;
//	}
//	return false;
//}

namespace It = UC::Iterators;


void Main()
{	
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);

	UC::TArray<float> MyFloatingArray;
	const UC::TArray<float> MyOtherArray;

	if (MyFloatingArray != MyOtherArray)
	{
		for (float& Value : MyFloatingArray)
		{
			Value = 6.9f;
		}

		for (float& Value : MyOtherArray)
		{
			Value = 9.6f;
		}
	}

	auto It = UC::begin(MyOtherArray);

	UC::TSet<float> MySet;

	for (float& FltRef : MySet)
	{
		std::cout << FltRef << std::endl;
	}
	
	UC::FString Str;
	UC::FString StrInt(0x69);
	UC::FString StrWChar0(L"Hell world!");
	const UC::FString StrWChar1(L"Some world!");

	if (StrWChar0 != StrWChar1)
	{
		for (wchar_t Char : StrWChar0)
		{
			std::wcout << Char << L", ";
		}
	}

	for (int i = 0; i < StrWChar0.Num(); i++)
	{
		std::wcout << StrWChar0[i] << L", ";
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