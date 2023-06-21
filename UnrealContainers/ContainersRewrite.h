// Copyright 1998-2023 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <string>


namespace UC /*UnrealContainers*/
{
	typedef int8_t int8;
	typedef int16_t int16;
	typedef int32_t int32;
	typedef int64_t int64;

	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;

	namespace FMemory
	{
		inline void* (*Malloc)(int32 Size, int32 Alignment) = nullptr;
		inline void* (*Realloc)(void* Memory, int64 NewSize, uint32 Alignment) = nullptr; // nullptr safe
		inline void  (*Free)(void* Memory) = nullptr; // nullptr safe

		inline void Init(void* M, void* R, void* F)
		{
			Malloc = reinterpret_cast<decltype(Malloc)>(M);
			Realloc = reinterpret_cast<decltype(Realloc)>(R);
			Free = reinterpret_cast<decltype(Free)>(F);
		}
	}

	namespace UEHelperFunctions
	{
		inline uint32 FloorLog2(uint32 Value)
		{
			uint32 pos = 0;
			if (Value >= 1 << 16) { Value >>= 16; pos += 16; }
			if (Value >= 1 << 8)  { Value >>= 8; pos += 8; }
			if (Value >= 1 << 4)  { Value >>= 4; pos += 4; }
			if (Value >= 1 << 2)  { Value >>= 2; pos += 2; }
			if (Value >= 1 << 1)  { pos += 1; }
			return pos;
		}

		inline uint32 CountLeadingZeros(uint32 Value)
		{
			if (Value == 0) 
				return 32;

			return 31 - FloorLog2(Value);
		}
	}

	template<typename ArrayDataType>
	class TArray;

	template<typename SparseArrayDataType>
	class TSparseArray;

	template<typename SetDataType>
	class TSet;

	template<typename KeyType, typename ValueType>
	class TMap;

	template <typename KeyType, typename ValueType>
	class TPair;

	namespace Iterators
	{
		class FSetBitIterator;

		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		class TContainerIterator;


		template<typename ArrayType>
		using TArrayIterator = TContainerIterator<int32, TArray, ArrayType>;

		template<typename SparseArrayType>
		using TSparseArrayIterator = TContainerIterator<FSetBitIterator, TSparseArray, SparseArrayType>;

		template<typename SetType>
		using TSetIterator = TContainerIterator<TSparseArrayIterator<typename TSet<SetType>::DataType>, TSet, SetType>;

		template<typename KeyType, typename ValueType>
		using TMapIterator = TContainerIterator<TSetIterator<typename TMap<KeyType, ValueType>::DataType>, TMap, KeyType, ValueType>;
	}


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
		inline       KeyType& Key() { return First; }
		inline const KeyType& Key() const { return First; }

		inline       ValueType& Value() { return Second; }
		inline const ValueType& Value() const { return Second; }
	};

	template<typename ArrayDataType>
	class TArray
	{
	public:
		using DataType = ArrayDataType;

	private:
		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		friend class Iterators::TContainerIterator;

		template<typename SparseArrayDataType>
		friend class TSparseArray;

	protected:
		static constexpr int32 TypeSize = sizeof(DataType);
		static constexpr int32 TypeAlign = alignof(DataType);

	protected:
		ArrayDataType* Data;
		int32 NumElements;
		int32 MaxElements;

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
		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return NumElements; }

		inline bool IsValidIndex(int32 Index) const { return Data && Index > 0 && Index < NumElements; }

		inline bool IsValid() const { return Data && NumElements > 0 && MaxElements > 0; }

	public:
		inline void Free() 
		{ 
			FMemory::Free(Data); 
			NumElements = 0; 
			MaxElements = 0; 
		}

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

	public:
		inline       ArrayDataType& operator[](int32 Index)       { VerifyIndex(Index); return Data[Index]; }
		inline const ArrayDataType& operator[](int32 Index) const { VerifyIndex(Index); return Data[Index]; }

		inline bool operator==(const TArray<ArrayDataType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<ArrayDataType>& Other) const { return Data != Other.Data; }

	public:
		template<typename T> friend Iterators::TArrayIterator<T> begin(const TArray& Array);
		template<typename T> friend Iterators::TArrayIterator<T> end  (const TArray& Array);

	private:
		inline const auto& GetDataRef() const { return Data; }
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
		inline int32 Num() const { return NumBits; }
		inline int32 Max() const { return MaxBits; }

		inline uint32* GetData() const { return (uint32*)Data.GetAllocation(); }

		inline bool IsValidIndex(int32 Index) const { return Index > 0 && Index < NumBits; }

		inline bool IsValid() const { return GetData() && NumBits > 0; }

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline bool operator[](int32 Index) const { VerifyIndex(Index); return GetData()[Index / NumBitsPerDWORD] & (1 << (Index & (NumBitsPerDWORD - 1))); }

		inline bool operator==(const FBitArray& Other) const { return NumBits == Other.NumBits && GetData() == Other.GetData(); }
		inline bool operator!=(const FBitArray& Other) const { return NumBits != Other.NumBits || GetData() != Other.GetData(); }

	public:
		friend Iterators::FSetBitIterator begin(const FBitArray& Array);
		friend Iterators::FSetBitIterator end  (const FBitArray& Array);
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
	public:
		using DataType = TSparseArrayElementOrFreeListLink<TAlignedBytes<sizeof(SparseArrayDataType), alignof(SparseArrayDataType)>>;

	private:
		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		friend class Iterators::TContainerIterator;

	private:
		TArray<DataType> Data; // FElementOrFreeListLink
		FBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		inline int32 Num() const { return Data.Num(); }
		inline int32 Max() const { return Data.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Data.IsValidIndex(Index) && AllocationFlags[Index]; }

		inline bool IsValid() const { return Data.IsValid() && AllocationFlags.IsValid(); }

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline       SparseArrayDataType& operator[](int32 Index)       { VerifyIndex(Index); return *(SparseArrayDataType*)&Data.GetUnsafe(Index).ElementData; }
		inline const SparseArrayDataType& operator[](int32 Index) const { VerifyIndex(Index); return *(SparseArrayDataType*)&Data.GetUnsafe(Index).ElementData; }

		inline bool operator==(const TSparseArray<SparseArrayDataType>& Other) const { return Data.operator==(Other.Data); }
		inline bool operator!=(const TSparseArray<SparseArrayDataType>& Other) const { return Data.operator!=(Other.Data); }

	public:
		template<typename T> friend Iterators::TSparseArrayIterator<T> begin(const TSparseArray& Array);
		template<typename T> friend Iterators::TSparseArrayIterator<T> end  (const TSparseArray& Array);

	private:
		inline const auto& GetDataRef() const { return AllocationFlags; }
	};


	template<typename T>
	class SetElement
	{
	private:
		template<typename SetDataType>
		friend class TSet;

	private:
		T Value;
		int32 HashNextId;
		int32 HashIndex;
	};

	template<typename SetDataType>
	class TSet
	{
	public:
		using DataType = SetElement<SetDataType>;

	private:
		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		friend class Iterators::TContainerIterator;

	private:
		TSparseArray<DataType> Elements;
		TInlineAllocator<1>::ForElementType<int32> Hash;
		int32 HashSize;

	public:
		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline       SetDataType& operator[] (int32 Index)       { return Elements[Index].Value; }
		inline const SetDataType& operator[] (int32 Index) const { return Elements[Index].Value; }
		
		inline bool operator==(const TSet<SetDataType>& Other) const { return Elements.operator==(Other.Elements); }
		inline bool operator!=(const TSet<SetDataType>& Other) const { return Elements.operator!=(Other.Elements); }

	public:
		template<typename T> friend Iterators::TSetIterator<T> begin(const TSet& Set);
		template<typename T> friend Iterators::TSetIterator<T> end  (const TSet& Set);

	private:
		inline const auto& GetDataRef() const { return Elements; }
	};


	template<typename KeyType, typename ValueType>
	class TMap
	{
	public:
		using DataType = TPair<KeyType, ValueType>;

	private:
		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		friend class Iterators::TContainerIterator;

	private:
		TSet<DataType> Elements;

	public:
		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		inline decltype(auto) Find(const KeyType& Key, bool(*Equals)(const KeyType& L, const KeyType& R))
		{
			for (auto It = begin(*this); It != end(*this); ++It)
			{
				if (Equals(It->Key(), Key))
					return It;
			}

			return end(*this);
		}

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline       DataType& operator[] (int32 Index)       { return Elements[Index]; }
		inline const DataType& operator[] (int32 Index) const { return Elements[Index]; }

		inline bool operator==(const TMap<KeyType, ValueType>& Other) const { return Elements.operator==(Other.Elements); }
		inline bool operator!=(const TMap<KeyType, ValueType>& Other) const { return Elements.operator!=(Other.Elements); }

	public:
		template<typename T0, typename T1> friend Iterators::TMapIterator<T0, T1> begin(const TMap& Map);
		template<typename T0, typename T1> friend Iterators::TMapIterator<T0, T1> end(const TMap& Map);

	private:
		inline const auto& GetDataRef() const { return Elements; }
	};

	namespace Iterators
	{
		class FRelativeBitReference
		{
		protected:
			static constexpr int32 NumBitsPerDWORD = 32;
			static constexpr int32 NumBitsPerDWORDLogTwo = 5;

		public:
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
				UnvisitedBitMask &= ~this->Mask;

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

				uint32 RemainingBitMask = ArrayData[this->WordIndex] & UnvisitedBitMask;
				while (!RemainingBitMask)
				{
					++this->WordIndex;
					BaseBitIndex += NumBitsPerDWORD;
					if (this->WordIndex > LastWordIndex)
					{
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


		template<bool bWrapInOuter, template<typename...> class OuterType, typename... Types>
		struct TypeSelector { };

		template<template<typename...> class OuterType, typename T>
		struct TypeSelector<false, OuterType, T> { using Type = T; };

		template<template<typename...> class OuterType, typename T>
		struct TypeSelector<true, OuterType, T> { using Type = OuterType<T>; };

		template<bool bWrapInOuter, template<typename...> class OuterType, typename T1, typename T2>
		struct TypeSelector<bWrapInOuter, OuterType, T1, T2> { using Type = OuterType<T1, T2>; };


		template<typename SuperIterator, template<typename...> class OuterType, typename... Types>
		class TContainerIterator
		{
		private:
			using OwnType = TContainerIterator<SuperIterator, OuterType, Types...>;
			using DataType = typename TypeSelector<false, TPair, Types...>::Type;
			using ContainerType = typename TypeSelector<true, OuterType, Types...>::Type;

		private:
			static constexpr bool bIsTrivialSuperIt = std::is_trivial_v<SuperIterator>;

		private:
			template<typename DataStructureType>
			static SuperIterator Make(const DataStructureType& Container, int32 Index)
			{
				if constexpr (!std::is_trivial_v<SuperIterator>) {
					return SuperIterator(Container, Index);
				} else {
					return Index;
				}
			}

		private:
			SuperIterator SuperIt;
			ContainerType& IteratedContainer;

		public:
			TContainerIterator() = default;

			TContainerIterator(const ContainerType& Container, uint32 CurrentIndex = 0)
				: IteratedContainer(const_cast<ContainerType&>(Container))
				, SuperIt(Make<decltype(Container.GetDataRef())>(Container.GetDataRef(), CurrentIndex))
			{
			}

		public:
			inline int32 GetIndex() { if constexpr (!bIsTrivialSuperIt) { return SuperIt.GetIndex(); } else { return SuperIt; } }

		public:
			inline       DataType& operator*()       { return IteratedContainer[GetIndex()]; }
			inline const DataType& operator*() const { return IteratedContainer[GetIndex()]; }

			inline       DataType* operator->()       { return &IteratedContainer[GetIndex()]; }
			inline const DataType* operator->() const { return &IteratedContainer[GetIndex()]; }

			inline TContainerIterator& operator++() { ++SuperIt; return *this; }
			inline TContainerIterator& operator--() { --SuperIt; return *this; }

			inline bool operator==(const TContainerIterator& Other) const { return SuperIt == Other.SuperIt && IteratedContainer == Other.IteratedContainer; }
			inline bool operator!=(const TContainerIterator& Other) const { return SuperIt != Other.SuperIt || IteratedContainer != Other.IteratedContainer; }
		};
	}

	Iterators::FSetBitIterator begin(const FBitArray& Array) { return Iterators::FSetBitIterator(Array, 0); }
	Iterators::FSetBitIterator end(const FBitArray& Array) { return Iterators::FSetBitIterator(Array, Array.Num()); }

	template<typename T> Iterators::TArrayIterator<T> begin(const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, 0); }
	template<typename T> Iterators::TArrayIterator<T> end  (const TArray<T>& Array) { return Iterators::TArrayIterator<T>(Array, Array.Num()); }

	template<typename T> Iterators::TSparseArrayIterator<T> begin(const TSparseArray<T>& Array) { return Iterators::TSparseArrayIterator<T>(Array, 0); };
	template<typename T> Iterators::TSparseArrayIterator<T> end  (const TSparseArray<T>& Array) { return Iterators::TSparseArrayIterator<T>(Array, Array.Num()); }

	template<typename T> Iterators::TSetIterator<T> begin(const TSet<T>& Set) { return Iterators::TSetIterator<T>(Set, 0); };;
	template<typename T> Iterators::TSetIterator<T> end  (const TSet<T>& Set) { return Iterators::TSetIterator<T>(Set, Set.Num()); };

	template<typename T0, typename T1> Iterators::TMapIterator<T0, T1> begin(const TMap<T0, T1>& Map) { return Iterators::TMapIterator<T0, T1>(Map, 0); }
	template<typename T0, typename T1> Iterators::TMapIterator<T0, T1> end  (const TMap<T0, T1>& Map) { return Iterators::TMapIterator<T0, T1>(Map, Map.Num()); }
}



