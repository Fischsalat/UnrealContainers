#pragma once
#include "Containers.h"
#include <string>
#include <Windows.h>

using namespace UE;

static class FUObjectArray* GObjects;

class FName
{
public:
	int32 comparisonIndex;
	int32 number;
	inline FString ToFString() const
	{
		if (!this)
			return L"";

		static auto ToStr2 = reinterpret_cast<void(*)(const FName*, FString&)>(uintptr_t(GetModuleHandle(0)) + 0x117C420);

		FString outStr;
		ToStr2(this, outStr);

		return outStr;
	}

	inline std::string ToString() const
	{
		if (!this)
		{
			return "";
		}

		static auto ToStr = reinterpret_cast<void(*)(const FName*, FString&)>(uintptr_t(GetModuleHandle(0)) + 0x117C420);

		FString outStr;
		ToStr(this, outStr);

		std::string outName = outStr.ToString();
		outStr.Free();

		if (number > 0)
			outName += '_' + std::to_string(number);

		auto pos = outName.rfind('/');
		if (pos == std::string::npos)
			return outName;

		return outName.substr(pos + 1);
	}

	inline bool operator==(FName other)
	{
		return comparisonIndex == other.comparisonIndex;
	}
	inline bool operator!=(FName other)
	{
		return !(operator==(other));
	}
};

class UObject
{
public:
	void** vft;
	int32 flags;
	int32 internalIndex;
	UObject* privateClass;
	FName name;
	UObject* outer;

	inline std::string GetName() const
	{
		return this ? name.ToString() : "None";
	}

	inline std::string GetFullName() const
	{
		if (this && privateClass)
		{
			std::string temp;

			for (UObject* outr = outer; outr != nullptr; outr = outr->outer)
			{
				temp = outer->GetName() + "." + temp;
			}

			std::string retName = privateClass->GetName() + " ";
			retName += temp;
			retName += GetName();

			return retName;
		}
		return "";
	}
};

class FUObjectItem
{
public:
	UObject* object;
	int32 indexAndFlags;
	int32 serialNumber;
	int8 paddingTEST[0x8];
};

class TUObjectArray
{
public:
	FUObjectItem* objects;
	int32 maxNumElements;
	int32 numElements;
};

class FUObjectArray
{
public:
	uint8 pad[0x10];
	TUObjectArray objObjects;

	inline UObject* ByIndex(int32 index)
	{
		return objObjects.objects[index].object;
	}
	inline int32 Num()
	{
		return objObjects.numElements;
	}
	inline UObject* Find(std::string FullName)
	{
		for (int i = 0; i < Num(); i++)
		{
			UObject* current = ByIndex(i);

			if (current && current->GetName() == FullName)
				return current;
		}
		return nullptr;
	}
};