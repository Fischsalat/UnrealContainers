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

	namespace UEHelperFunctions
	{
		inline uint32 FloorLog2(uint32 Value)
		{
			uint32 pos = 0;
			if (Value >= 1 << 16) { Value >>= 16; pos += 16; }
			if (Value >= 1 << 8) { Value >>= 8; pos += 8; }
			if (Value >= 1 << 4) { Value >>= 4; pos += 4; }
			if (Value >= 1 << 2) { Value >>= 2; pos += 2; }
			if (Value >= 1 << 1) { pos += 1; }
			return pos;
		}

		inline uint32 CountLeadingZeros(uint32 Value)
		{
			if (Value == 0) return 32;
			return 31 - FloorLog2(Value);
		}
	}

	namespace Iterators
	{
		class FSetBitIterator;

		template<template<typename...> class OuterType, typename T1, typename T2 = void>
		class TContainerIterator;

		template<typename SuperIterator, template<typename...> class OuterType, typename T1, typename T2 = void>
		class TContainerIteratorSuperTest;
	}

	template<typename ArrayDataType>
	class TArray
	{
	private:
		template<template<typename...> class OuterType, typename T1, typename T2 = void>
		friend class Iterators::TContainerIterator;

		template<typename SuperIterator, template<typename...> class OuterType, typename T1, typename T2 = void>
		friend class Iterators::TContainerIteratorSuperTest;

		template<typename SparseArrayDataType>
		friend class TSparseArray;

	public:
		using TArrayType = ArrayDataType;

	protected:
		static constexpr int32 TypeSize = sizeof(ArrayDataType);
		static constexpr int32 TypeAlign = alignof(ArrayDataType);

	public:
		TArray()
			: Data(nullptr), NumElements(0), MaxElements(0)
		{
		}

		TArray(uint32 InitSize)
			: Data((ArrayDataType*)FMemory::Malloc(InitSize * TypeSize, TypeAlign)), NumElements(0), MaxElements(InitSize)
		{
		}

	public:
		inline bool IsValidIndex(int32 Index) const { return Data && Index > 0 && Index < NumElements; }

		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return NumElements; }

		inline void Free() { FMemory::Free(Data); NumElements = 0; MaxElements = 0; }

		inline void Add(const ArrayDataType& Element)
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

		inline void Reserve(int32 Count) { MaxElements += Count; FMemory::Realloc(Data, MaxElements, TypeAlign); }

		inline void VerifyIndex (int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		inline       ArrayDataType& GetUnsafe(int32 Index)       { return Data[Index]; }
		inline const ArrayDataType& GetUnsafe(int32 Index) const { return Data[Index]; }

		inline const void* GetSubContainerForIterator() const { return Data; }

	public:
		inline       ArrayDataType& operator[](int32 Index)       { VerifyIndex(Index); return Data[Index]; }
		inline const ArrayDataType& operator[](int32 Index) const { VerifyIndex(Index); return Data[Index]; }

		inline bool operator==(const TArray<ArrayDataType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<ArrayDataType>& Other) const { return Data != Other.Data; }

	public:
		//template<typename T> friend const Iterators::TArrayIterator<ArrayDataType> begin(const TArray& Array);
		//template<typename T> friend const Iterators::TArrayIterator<ArrayDataType> end  (const TArray& Array);
		template<typename T> friend Iterators::TContainerIterator<TArray, ArrayDataType> begin(const TArray& Array);
		template<typename T> friend Iterators::TContainerIterator<TArray, ArrayDataType> end(const TArray& Array);

	protected:
		ArrayDataType* Data;
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

	template<int32 Size, uint32 Alignment>
	struct TAlignedBytes
	{
		alignas(Alignment) uint8 Pad[Size];
	};

	template<uint32 NumInlineElements>
	class TInlineAllocator
	{
	public:
		template<typename ElementType>
		class ForElementType
		{
		private:
			static constexpr int32 TypeSize = sizeof(ElementType);
			static constexpr int32 TypeAlign = alignof(ElementType);

		public:
			inline ElementType* GetAllocation() const { return SecondaryData ? SecondaryData : (ElementType*)InlineData; }

		private:
			TAlignedBytes<TypeSize, TypeAlign> InlineData[NumInlineElements];
			ElementType* SecondaryData;
		};
	};

	class FBitArray
	{
	protected:
		static constexpr int32 NumBitsPerDWORD = 32;
		static constexpr int32 NumBitsPerDWORDLogTwo = 5;

	private:
		TInlineAllocator<4>::ForElementType<int32> Data;
		int32 NumBits;
		int32 MaxBits;

	public:
		inline bool IsValidIndex(int32 Index) const { return Index > 0 && Index < NumBits; }

		inline int32 Num() const { return NumBits; }
		inline int32 Max() const { return MaxBits; }

		inline uint32* GetData() const { return (uint32*)Data.GetAllocation(); }

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline bool operator[](int32 Index) const { VerifyIndex(Index); return GetData()[Index / NumBitsPerDWORD] & (1 << (Index & (NumBitsPerDWORD - 1))); }
	};

	template<typename ElementType>
	union TSparseArrayElementOrFreeListLink
	{
		ElementType ElementData;

		struct
		{
			int32 PrevFreeIndex;
			int32 NextFreeIndex;
		};
	};

	template<typename SparseArrayDataType>
	class TSparseArray
	{
	private:
		template<template<typename...> class OuterType, typename T1, typename T2 = void>
		friend class Iterators::TContainerIterator;

		template<typename SuperIterator, template<typename...> class OuterType, typename T1, typename T2 = void>
		friend class Iterators::TContainerIteratorSuperTest;
		
	private:
		static constexpr int32 TypeSize = sizeof(SparseArrayDataType);
		static constexpr int32 TypeAlign = alignof(SparseArrayDataType);

	private:
		using FElementOrFreeListLink = TSparseArrayElementOrFreeListLink<TAlignedBytes<TypeSize, TypeAlign>>;

	private:
		TArray<FElementOrFreeListLink> Data;
		FBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		inline bool IsValidIndex(int32 Index) const { return Data.IsValidIndex(Index) && AllocationFlags[Index]; }

		inline int32 Num() { return Data.Num(); }
		inline int32 Max() { return Data.Max(); }

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		inline const FBitArray& GetSubContainerForIterator() const { return AllocationFlags; }

	public:
		inline       SparseArrayDataType& operator[](int32 Index)       { VerifyIndex(Index); return *(SparseArrayDataType*)&Data.GetUnsafe(Index).ElementData; }
		inline const SparseArrayDataType& operator[](int32 Index) const { VerifyIndex(Index); return *(SparseArrayDataType*)&Data.GetUnsafe(Index).ElementData; }

		inline bool operator==(const TSparseArray<SparseArrayDataType>& Other) const { return Data.operator==(Other.Data); }
		inline bool operator!=(const TSparseArray<SparseArrayDataType>& Other) const { return Data.operator==(Other.Data); }
	};

	template <typename KeyType, typename ValueType>
	class TPair
	{
	private:
		KeyType First;
		ValueType Second;

	public:
		TPair(KeyType Key, ValueType Value)
			: First(Key)
			, Second(Value)
		{
		}

	public:
		FORCEINLINE KeyType& Key()
		{
			return First;
		}
		FORCEINLINE const KeyType& Key() const
		{
			return First;
		}
		FORCEINLINE ValueType& Value()
		{
			return Second;
		}
		FORCEINLINE const ValueType& Value() const
		{
			return Second;
		}
	};

	template<typename SetDataType>
	class TSet
	{

	};


	template<typename KeyType, typename ValueType>
	class TMap
	{
		TSet<TPair<KeyType, ValueType>> Elements;
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



		class FRelativeBitReference
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		public:
			FRelativeBitReference() = default;

			inline explicit FRelativeBitReference(int32 BitIndex)
				: WordIndex(BitIndex >> NumBitsPerDWORDLogTwo)
				, Mask(1 << (BitIndex & (NumBitsPerDWORD - 1)))
			{
			}

			int32  WordIndex;
			uint32 Mask;
		};

		class FSetBitIterator : public FRelativeBitReference
		{
		private:
			const FBitArray& Array;

			uint32 UnvisitedBitMask;
			int32 CurrentBitIndex;
			int32 BaseBitIndex;

		public:
			FSetBitIterator() = default;

			explicit FSetBitIterator(const FBitArray& InArray, int32 StartIndex = 0)
				: FRelativeBitReference(StartIndex)
				, Array(InArray)
				, UnvisitedBitMask((~0U) << (StartIndex & (NumBitsPerDWORD - 1)))
				, CurrentBitIndex(StartIndex)
				, BaseBitIndex(StartIndex & ~(NumBitsPerDWORD - 1))
			{
				if (StartIndex != Array.Num())
					FindFirstSetBit();
			}

		public:
			inline FSetBitIterator& operator++()
			{
				// Mark the current bit as visited.
				UnvisitedBitMask &= ~this->Mask;

				// Find the first set bit that hasn't been visited yet.
				FindFirstSetBit();

				return *this;
			}

			inline explicit operator bool() const { return CurrentBitIndex < Array.Num(); }

			inline bool operator==(const FSetBitIterator& Rhs) const { return CurrentBitIndex == Rhs.CurrentBitIndex && &Array == &Rhs.Array; }
			inline bool operator!=(const FSetBitIterator& Rhs) const { return CurrentBitIndex != Rhs.CurrentBitIndex || &Array != &Rhs.Array; }

		public:
			inline int32 GetIndex() { return CurrentBitIndex; }

			void FindFirstSetBit()
			{
				const uint32* ArrayData = Array.GetData();
				const int32   ArrayNum = Array.Num();
				const int32   LastWordIndex = (ArrayNum - 1) / NumBitsPerDWORD;

				// Advance to the next non-zero uint32.
				uint32 RemainingBitMask = ArrayData[this->WordIndex] & UnvisitedBitMask;
				while (!RemainingBitMask)
				{
					++this->WordIndex;
					BaseBitIndex += NumBitsPerDWORD;
					if (this->WordIndex > LastWordIndex)
					{
						// We've advanced past the end of the array.
						CurrentBitIndex = ArrayNum;
						return;
					}

					RemainingBitMask = ArrayData[this->WordIndex];
					UnvisitedBitMask = ~0;
				}

				const uint32 NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

				this->Mask = NewRemainingBitMask ^ RemainingBitMask;

				CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - UEHelperFunctions::CountLeadingZeros(this->Mask);

				if (CurrentBitIndex > ArrayNum)
					CurrentBitIndex = ArrayNum;

			}
		};

		template<template<typename...> class OuterType, typename T1, typename T2 = void>
		class TContainerIterator
		{
		public:
			using ContainerType = OuterType<T1>;
			using DataType = std::conditional_t<std::is_same_v<T2, void>, T1, TPair<T1, T2>>;

		private:
			uint32 CurrentIndex;
			ContainerType& IteratedContainer;

		public:
			TContainerIterator(const ContainerType& Container)
				: IteratedContainer(const_cast<ContainerType&>(Container))
				, CurrentIndex(Container.Num())
			{
			}
			TContainerIterator(const ContainerType& Container, uint32 CurrentIndex)
				: IteratedContainer(const_cast<ContainerType&>(Container))
				, CurrentIndex(CurrentIndex)
			{
			}

		public:
			inline       DataType& operator*()       { return IteratedContainer[CurrentIndex]; }
			inline const DataType& operator*() const { return IteratedContainer[CurrentIndex]; }

			inline       DataType* operator->()       { return &IteratedContainer[CurrentIndex]; }
			inline const DataType* operator->() const { return &IteratedContainer[CurrentIndex]; }

			inline TContainerIterator& operator++() { ++CurrentIndex; return *this; }
			inline TContainerIterator& operator--() { --CurrentIndex; return *this; }

			inline bool operator==(const TContainerIterator& Other) const { return CurrentIndex == Other.CurrentIndex && IteratedContainer == Other.IteratedContainer; }
			inline bool operator!=(const TContainerIterator& Other) const { return CurrentIndex != Other.CurrentIndex || IteratedContainer != Other.IteratedContainer; }
		};

		template<typename T>
		class ConstructionHelper
		{
		public:
			template<typename DataStructureType>
			static T Make(const DataStructureType& Container, int32 Index)
			{
				if constexpr (!std::is_trivial_v<T>)
				{
					return T(Container, Index);
				}
				else
				{
					return Index;
				}
			}
		};

		template<typename SuperIterator, template<typename...> class OuterType, typename T1, typename T2 = void>
		class TContainerIteratorSuperTest
		{
		public:
			using ContainerType = OuterType<T1>;
			using DataType = std::conditional_t<std::is_same_v<T2, void>, T1, TPair<T1, T2>>;

		private:
			static constexpr bool bIsTrivialSuperIt = std::is_trivial_v<SuperIterator>;

		private:
			SuperIterator SuperIt;
			ContainerType& IteratedContainer;

		public:
			TContainerIteratorSuperTest() = default;

			TContainerIteratorSuperTest(const ContainerType& Container)
				: IteratedContainer(const_cast<ContainerType&>(Container))
				, SuperIt(ConstructionHelper<SuperIterator>::template Make<decltype(Container.GetSubContainerForIterator())>(Container.GetSubContainerForIterator(), 0))
			{
			}

			TContainerIteratorSuperTest(const ContainerType& Container, uint32 CurrentIndex)
				: IteratedContainer(const_cast<ContainerType&>(Container))
				, SuperIt(ConstructionHelper<SuperIterator>::template Make<decltype(Container.GetSubContainerForIterator())>(Container.GetSubContainerForIterator(), CurrentIndex))
			{
			}

		public:
			//inline       DataType& operator*()       { return IteratedContainer[CurrentIndex]; }
			//inline const DataType& operator*() const { return IteratedContainer[CurrentIndex]; }

			inline DataType& operator*()
			{
				if constexpr (!bIsTrivialSuperIt)
				{
					return IteratedContainer[SuperIt.GetIndex()];
				}
				else
				{
					return IteratedContainer[SuperIt];
				}
			}

			inline int32 GetIndex()
			{
				if constexpr (!bIsTrivialSuperIt)
				{
					return SuperIt.GetIndex();
				}
				else
				{
					return SuperIt;
				}
			}

			//inline const DataType& operator*() const { return IteratedContainer[CurrentIndex]; }

			//inline       DataType* operator->()       { return &IteratedContainer[CurrentIndex]; }
			//inline const DataType* operator->() const { return &IteratedContainer[CurrentIndex]; }

			inline TContainerIteratorSuperTest& operator++() { ++SuperIt; return *this; }
			inline TContainerIteratorSuperTest& operator--() { --SuperIt; return *this; }

			inline bool operator==(const TContainerIteratorSuperTest& Other) const { return SuperIt == Other.SuperIt && IteratedContainer == Other.IteratedContainer; }
			inline bool operator!=(const TContainerIteratorSuperTest& Other) const { return SuperIt != Other.SuperIt || IteratedContainer != Other.IteratedContainer; }
		};
	}

	//template<typename T> Iterators::TArrayIterator<T> begin(const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, 0); }
	//template<typename T> Iterators::TArrayIterator<T> end  (const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, Array.Num()); }


	template<typename T> Iterators::TContainerIterator<TArray, T> begin(const TArray<T>& Array) { return Iterators::TContainerIterator<TArray, T>(Array, 0); }
	template<typename T> Iterators::TContainerIterator<TArray, T> end(const TArray<T>& Array)   { return Iterators::TContainerIterator<TArray, T>(Array, Array.Num()); }
}

