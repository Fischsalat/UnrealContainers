// Copyright 1998-2023 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <string>

namespace UC /*UnrealContainers*/
{
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	namespace FMemory
	{
		inline void* (*Malloc)(int32 Size, int32 Alignment) = nullptr;
		inline void* (*Realloc)(void* Memory, int64 NewSize, uint32 Alignment) = nullptr; // nullptr safe
		inline void  (*Free)(void* Memory) = nullptr; // nullptr safe
	}

	namespace Iterators
	{
		class FBitArrayIterator;

		template<typename T> class TArrayIterator;
		template<typename T> class TSparseArrayIterator;
		template<typename T> class TSetIterator;
		template<typename T> class TMapIterator;
	}

	template<typename DataType>
	class TArray
	{
	protected:
		static constexpr int32 TypeSize = sizeof(DataType);
		static constexpr int32 TypeAlign = alignof(DataType);

	public:
		TArray()
			: Data(nullptr), NumElements(0), MaxElements(0)
		{
		}

		TArray(uint32 InitSize)
			: Data((DataType*)FMemory::Malloc(InitSize * TypeSize, TypeAlign)), NumElements(0), MaxElements(InitSize)
		{
		}

	public:
		inline bool IsValidIndex(int32 Index) const { return Data && Index > 0 && Index < NumElements; }

		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return NumElements; }

		inline void Free() { FMemory::Free(Data); NumElements = 0; MaxElements = 0; }

		inline void Add(const DataType& Element)
		{
			if (GetSlack() > 0)
				Reserve(3);

			Data[NumElements] = Element;
			NumElements++;
		}

		inline void Remove(int32 Index)
		{
			if (!IsValidIndex(Index))
				return;
			
			NumElements--;

			for (int i = Index; i < NumElements; i++)
			{
				Data[i] = Data[i + 1];
			}
		}

		inline void Clear()
		{
			memset(Data, 0, NumElements * TypeSize);
			NumElements = 0;
		}

	private:
		inline int32 GetSlack() const { return MaxElements - NumElements; }

		inline void Reserve(int32 Count) { MaxElements += Count; FMemory::Realloc(Data, MaxElements, alignof(DataType)); }

		inline void VerifyIndex (int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline       DataType& operator[](int32 Index)       { VerifyIndex(Index); return Data[Index]; }
		inline const DataType& operator[](int32 Index) const { VerifyIndex(Index); return Data[Index]; }

		inline bool operator==(const TArray<DataType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<DataType>& Other) const { return Data != Other.Data; }

	public:
		template<typename T> friend const Iterators::TArrayIterator<DataType> begin(const TArray& Array);
		template<typename T> friend const Iterators::TArrayIterator<DataType> end  (const TArray& Array);

	protected:
		DataType* Data;
		int32 NumElements;
		int32 MaxElements;
	};


	class FString : public TArray<wchar_t>
	{
	public:
		FString() = default;

		FString(uint32 InitSize)
			: TArray(InitSize)
		{
		}

		FString(const wchar_t* Str)
		{
			MaxElements = NumElements = (int32)(Str ? std::wcslen(Str) + 1 : 0);
			Data = nullptr;

			if (NumElements)
			{
				Data = (wchar_t*)FMemory::Malloc(NumElements * TypeSize, TypeAlign); 
				
				memcpy(Data, Str, (int64)NumElements * TypeSize);
			}
		}

	public:
		inline std::string ToString()
		{
			if (*this)
			{
				std::wstring WData(Data);
#pragma warning(suppress: 4244)
				return std::string(WData.begin(), WData.end());
			}

			return "";
		}

		inline std::wstring ToWString() 
		{
			if (*this)
				return std::wstring(Data);

			return L"";
		}

	public:
		inline explicit operator bool() const { return Data && NumElements > 0; };

		inline bool operator==(const FString& Other) const { return Other ? wcscmp(Data, Other.Data) == 0 : false; }
		inline bool operator!=(const FString& Other) const { return Other ? wcscmp(Data, Other.Data) != 0 : true;  }
	};

	namespace Iterators
	{
		template<typename T>
		class TArrayIterator
		{
		private:
			using TArrayType = TArray<T>;
			using DataType = T;

		private:
			uint32 CurrentIndex;
			TArrayType& IteratedArray;

		public:
			TArrayIterator(const TArrayType& Array)
				: IteratedArray(const_cast<TArrayType&>(Array))
				, CurrentIndex(Array.Count)
			{
			}
			TArrayIterator(const TArrayType& Array, uint32 CurrentIndex)
				: IteratedArray(const_cast<TArrayType&>(Array))
				, CurrentIndex(CurrentIndex)
			{
			}

		public:
			inline       DataType& operator*()       { return IteratedArray[CurrentIndex]; }
			inline const DataType& operator*() const { return IteratedArray[CurrentIndex]; }

			inline       DataType* operator->()       { return &IteratedArray[CurrentIndex]; }
			inline const DataType* operator->() const { return &IteratedArray[CurrentIndex]; }

			inline TArrayIterator& operator++() { ++CurrentIndex; return *this; }
			inline TArrayIterator& operator--() { --CurrentIndex; return *this; }

			inline bool operator==(const TArrayIterator& Other) const { return CurrentIndex == Other.CurrentIndex && IteratedArray == Other.IteratedArray; }
			inline bool operator!=(const TArrayIterator& Other) const { return CurrentIndex != Other.CurrentIndex || IteratedArray != Other.IteratedArray; }
		};
	}

	template<typename T> const Iterators::TArrayIterator<T> begin(const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, 0); }
	template<typename T> const Iterators::TArrayIterator<T> end  (const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, Array.Num()); }
}

