#pragma once
#include <Windows.h>
#include <iostream>
#include <format>
#include "Utils.h"
//#include "Containers.h"

//#include "ContainersRewrite.h"

//#define WITH_ALLOCATOR 0

//#include "ContainersRewrite.h"

//using namespace UE;

#include "UnrealContainers.h"

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

//namespace It = UC::Iterators;


DWORD MainThread(HMODULE Module)
{
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONIN$", "r", stdin);
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);

	/* Sig is only good for a few versions, not universal */
	UC::FMemory::Init(FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 41 8B D8 48 8B 0D ? ? ? ? 48 8B FA 48 85 C9 75 ? E8"));

	{
		UC::TArray<int> MyArray(3);

		UC::TArray<int>& MyOtherArrayRef = MyArray;
		UC::TArray<int> MyOtherArrayClone = MyArray;

		UC::TArray<int> MyOtherArray(0x1);
		MyOtherArray = MyArray;

		UC::FString SomeStr = L"Hell world...";

		UC::FString MyNewString(0x10);
		MyNewString = SomeStr;

		UC::TMap<float, UC::uint64> MapyMap;
		UC::TMap<float, UC::uint64> Map2;

		MapyMap = std::move(Map2);
		MapyMap = Map2;

		for (const UC::TPair<float, UC::uint64>& Pair : MapyMap)
		{
			std::wcout << "Look at my value: " << Pair.Value() << std::endl;
		}

		for (wchar_t C : SomeStr)
		{
			std::wcout << C << "- ";
		}
		std::wcout << std::endl;

		UC::FString SomeClonedString = SomeStr;
		for (wchar_t C : SomeClonedString)
		{
			std::wcout << C << "+";
		}
		std::wcout << std::endl;
	}

	std::cout << std::endl;

	struct alignas(0x4) FName { UC::int32 CmpIdx, Number; };

	UC::TMap<FName, float>& SomeMap = *reinterpret_cast<UC::TMap<FName, float>*>(0x000002C2334020E0 + 0x7c8);

	for (const UC::TPair<FName, float>& Pairs : SomeMap)
	{
		std::cout << std::format("CmpIdx: 0x{:X}, Number: 0x{:X}, float: {}\n", Pairs.Key().CmpIdx, Pairs.Key().Number, Pairs.Value());
	}

	/*
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

	//UC::FMemory::Init((void*)39);

	UC::TMap<UC::FString, void*>& PluginStateMachines = *(UC::TMap<UC::FString, void*>*)(0);

	for (auto It = UC::begin(PluginStateMachines); It != UC::end(PluginStateMachines); ++It)
	{
		std::cout << "Key: " << It->Key().ToString() << std::endl;
	}

	auto FoundIt = PluginStateMachines.Find(L"this is a key", [](auto L, auto R) { return L == R; });

	static_assert(std::is_same_v<decltype(FoundIt), decltype(UC::end(PluginStateMachines))>, "Not same type!");


	if (FoundIt != UC::end(PluginStateMachines))
		FoundIt->Key() = L"Hell!";

	if (FoundIt != UC::begin(PluginStateMachines)) 
		FoundIt->Key() = L"Heaven!";

	if (FoundIt.GetIndex() == 40)
		++FoundIt;

	for (auto& Pair : PluginStateMachines)
	{
		std::cout << "Key: " << Pair.Key().ToString() << std::endl;
	}
	
	auto It = UC::begin(MyOtherArray);
	
	UC::TArray<float> MyArray;
	UC::TSparseArray<float> MySparseArray;
	UC::TSet<float> MySet;
	UC::TMap<size_t, void*> MyMap;
	
	auto Itrator = MyMap.Find(30ull, [](auto L, auto R) { return L == R; });
	
	for (UC::TPair<size_t, void*>& Pair : MyMap)
	{
		std::cout << Pair.Value() << std::endl;
		std::cout << Pair.Key() << std::endl;
	}

	for (float& FltRef : MyArray)
	{
		std::cout << FltRef << std::endl;
	}
	
	for (float& FltRef : MySparseArray)
	{
		std::cout << FltRef << std::endl;
	}
	
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
	*/

	while (true)
	{
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			fclose(stdout);
			if (f) fclose(f);
			FreeConsole();

			FreeLibraryAndExitThread(Module, 0);
		}

		Sleep(100);
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
		break;
	}
	case DLL_PROCESS_DETACH: {
		break;
	}
	}
	return TRUE;
}