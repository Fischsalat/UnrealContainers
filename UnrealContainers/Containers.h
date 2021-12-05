#pragma once

namespace UE
{
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;


	template<class ElementType>
	class TArray
	{
		friend struct FString;


	public:
		inline int Num() const
		{
			return Count;
		}
		inline int Max() const
		{
			return MaxElements;
		}
		inline int Slack() const
		{
			return MaxElements - Count;
		}
		inline void Reserve(const int NumElements)
		{
			Data = Slack() >= NumElements ? Data : (ElementType*)Realloc(Data, (MaxElements = Count + NumElements) * sizeof(ElementType), 0);
		}
		inline void Reset(int MinSizeAfterReset = 0)
		{
			if (MaxElements >= MinSizeAfterReset)
			{
				Count = 0;

			}
		}
		inline void RemoveAt(const int Index, const int Lenght)
		{

		}
		void Add(ElementType InputData...)
		{
			int Num = sizeof(InputData) / sizeof(ElementType);

			Reserve(Num);
			Data[Count] = InputData;
			Count += Num;
		};

		inline ElementType& operator[](int i)
		{
			return Data[i];
		};

		inline const ElementType& operator[](int i) const
		{
			return Data[i];
		};

		inline int MaxIndex()
		{
			return Count - 1;
		}

		class FBaseArrayIterator
		{
			int32 Index;
			TArray<ElementType>& ItArray;

		public:
			FBaseArrayIterator(TArray<ElementType>& Array)
				: ItArray(Array), Index(Array.max)
			{
			}
			FBaseArrayIterator(TArray<ElementType>& Array, int32 CurrentIndex)
				: ItArray(Array), Index(CurrentIndex)
			{
			}

			FORCEINLINE ElementType& operator*()
			{
				return ItArray[Index];
			}
			FORCEINLINE ElementType& operator->()
			{
				return ItArray[Index];
			}
			FORCEINLINE FBaseArrayIterator& operator++()
			{
				++Index;
				return *this;
			}
			FORCEINLINE bool operator==(const FBaseArrayIterator& other) const
			{
				return Index == other.Index;
			}
			FORCEINLINE bool operator!=(const FBaseArrayIterator& other) const
			{
				return Index != other.Index;
			}
		};

		inline FBaseArrayIterator begin()
		{
			return FBaseArrayIterator(*this, 0);
		}
		inline FBaseArrayIterator begin() const
		{
			return FBaseArrayIterator(*this, 0);
		}
		inline FBaseArrayIterator end()
		{
			return FBaseArrayIterator(*this);
		}
		inline FBaseArrayIterator end() const
		{
			return FBaseArrayIterator(*this);
		}

	private:
		ElementType* Data;
		int32_t Count;
		int32_t MaxElements;
	};

	struct FString : private TArray<wchar_t>
	{
		inline FString()
		{
		};

		FString(const wchar_t* other)
		{
			MaxElements = Count = *other ? std::wcslen(other) + 1 : 0;

			if (Count)
			{
				Data = const_cast<wchar_t*>(other);
			}
		};

		inline bool IsValid() const
		{
			return Data != nullptr;
		}

		const wchar_t* ToWString() const
		{
			return Data;
		}

		std::string ToString() const
		{
			auto length = std::wcslen(Data);

			std::string str(length, '\0');

			std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

			return str;
		}
		inline void Free()
		{
			static auto FreeInternal = reinterpret_cast<void(*)(void*)>(uintptr_t(GetModuleHandle(0)) + 0x10AC0F0);

			FreeInternal(Data);
			Data = nullptr;
			Count = 0;
			MaxElements = 0;
		}
	};

	//Allocates Elements directly where the instance is created rather than on the heap
	template<int32 NumElements>
	class TInlineAllocator
	{
	private:
		template<int32 Size, int32 Alignment>
		struct alignas(Alignment) TAlligendBytes
		{
			uint8 Pad[Size];
		};

		template<typename ElementType>
		struct TTypeCompatibleBytes : public TAlligendBytes<sizeof(ElementType), alignof(ElementType)>
		{
		};

	public:
		template<typename ElementType>
		class ForElementType
		{
		private:
			TTypeCompatibleBytes<ElementType> InlineData[NumElements];

			void* SecondaryData;

		public:
			FORCEINLINE ElementType& operator[](int32 Index)
			{
				return *(ElementType*)(&InlineData[Index]);
			}
			FORCEINLINE const ElementType& operator[](int32 Index) const
			{
				return *(ElementType*)(&InlineData[Index]);
			}

			FORCEINLINE ElementType& GetInlinElement(int32 Index)
			{
				return *(ElementType*)(&InlineData[Index]);
			}
			FORCEINLINE const ElementType& GetInlinElement(int32 Index) const
			{
				return *(ElementType*)(&InlineData[Index]);
			}
			FORCEINLINE ElementType& GetSecondaryElement(int32 Index)
			{
				return (ElementType)SecondaryData[Index];
			}
			FORCEINLINE const ElementType& GetSecondaryElement(int32 Index) const
			{
				return (ElementType)SecondaryData[Index];
			}
		};
	};

	class TBitArray
	{

	private:
		TInlineAllocator<4>::ForElementType<uint32> Data;
		int32 NumBits;
		int32 MaxBits;

		struct FRelativeBitReference
		{
		public:
			FORCEINLINE explicit FRelativeBitReference(int32 BitIndex)
				: DWORDIndex(BitIndex >> ((int32)5))
				, Mask(1 << (BitIndex & (((int32)32) - 1)))
			{
			}

			int32 DWORDIndex;
			uint32 Mask;
		};
	public:
		struct FBitReference
		{
			FORCEINLINE FBitReference(uint32& InData, uint32 InMask)
				: Data(InData), Mask(InMask)
			{
			}
			FORCEINLINE const FBitReference(const uint32& InData, const uint32 InMask)
				: Data(const_cast<uint32&>(InData)), Mask(InMask)
			{
			}

			FORCEINLINE operator bool() const
			{
				return (Data & Mask) != 0;
			}

		private:
			uint32& Data;
			uint32 Mask;
		};

	public:
		class FBitIterator : public FRelativeBitReference
		{
		public:
			int32 Index;
			const TBitArray& IteratedArray;

		public:
			FORCEINLINE const FBitIterator(const TBitArray& ToIterate, const int32 StartIndex) // Begin
				: IteratedArray(ToIterate), Index(StartIndex), FRelativeBitReference(StartIndex)
			{
			}
			FORCEINLINE const FBitIterator(const TBitArray& ToIterate) //End
				: IteratedArray(ToIterate), Index(ToIterate.NumBits), FRelativeBitReference(ToIterate.NumBits)
			{
			}

			FORCEINLINE explicit operator bool()
			{
				return Index < IteratedArray.Num();
			}
			FORCEINLINE FBitIterator& operator++()
			{
				++Index;
				this->Mask <<= 1;
				if (!this->Mask)
				{
					this->Mask = 1;
					++this->DWORDIndex;
				}
				return *this;
			}
			FORCEINLINE bool operator*() const
			{
				return (bool)FBitReference(IteratedArray.Data[this->DWORDIndex], this->Mask);
			}
			FORCEINLINE bool operator==(const FBitIterator& OtherIt) const
			{
				return Index == OtherIt.Index;
			}
			FORCEINLINE bool operator!=(const FBitIterator& OtherIt) const
			{
				return Index != OtherIt.Index;
			}

			FORCEINLINE int32 GetIndex() const
			{
				return Index;
			}
		};

	public:
		FORCEINLINE FBitIterator begin()
		{
			return FBitIterator(*this, 0);
		}
		FORCEINLINE const FBitIterator begin() const
		{
			return FBitIterator(*this, 0);
		}
		FORCEINLINE FBitIterator end()
		{
			return FBitIterator(*this);
		}
		FORCEINLINE const FBitIterator end() const
		{
			return FBitIterator(*this);
		}

		FORCEINLINE int32 Num() const
		{
			return NumBits;
		}
		FORCEINLINE int32 Max() const
		{
			return MaxBits;
		}
		FORCEINLINE bool IsSet(int32 Index) const
		{
			return *FBitIterator(*this, Index);
		}
	};

	template<typename ElementType>
	union TSparseArrayElementOrListLink
	{
		/** If the element is allocated, its value is stored here. */
		ElementType ElementData;

		struct
		{
			/** If the element isn't allocated, this is a link to the previous element in the array's free list. */
			int32 PrevFreeIndex;

			/** If the element isn't allocated, this is a link to the next element in the array's free list. */
			int32 NextFreeIndex;
		};
	};


	template<typename ArrayType>
	class TSparseArray
	{
	public:
		typedef TSparseArrayElementOrListLink<ArrayType> FSparseArrayElement;

	private:
		TArray<FSparseArrayElement> Data;
		TBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:

		class FBaseIterator
		{
		private:
			TSparseArray<ArrayType>& IteratedArray;
			TBitArray::FBitIterator BitArrayIt;

		public:
			FORCEINLINE FBaseIterator(const TSparseArray<ArrayType>& Array, const TBitArray::FBitIterator BitIterator)
				: IteratedArray(const_cast<TSparseArray<ArrayType>&>(Array)), BitArrayIt(const_cast<TBitArray::FBitIterator&>(BitIterator))
			{
			}

			FORCEINLINE TSparseArray<ArrayType>::FBaseIterator& operator++()
			{
				++BitArrayIt;
				return *this;
			}
			FORCEINLINE ArrayType& operator*()
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE const ArrayType& operator*() const
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE ArrayType& operator->()
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE const ArrayType& operator->() const
			{
				return IteratedArray[BitArrayIt.GetIndex()].ElementData;
			}
			FORCEINLINE bool operator==(const TSparseArray<ArrayType>::FBaseIterator& Other) const
			{
				return BitArrayIt == Other.BitArrayIt;
			}
			FORCEINLINE bool operator!=(const TSparseArray<ArrayType>::FBaseIterator& Other) const
			{
				return BitArrayIt != Other.BitArrayIt;
			}
		};

	public:
		FORCEINLINE TSparseArray<ArrayType>::FBaseIterator begin()
		{
			return TSparseArray<ArrayType>::FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE const TSparseArray<ArrayType>::FBaseIterator begin() const
		{
			return TSparseArray<ArrayType>::FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags, 0));
		}
		FORCEINLINE TSparseArray<ArrayType>::FBaseIterator end()
		{
			return TSparseArray<ArrayType>::FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}
		FORCEINLINE const TSparseArray<ArrayType>::FBaseIterator end() const
		{
			return TSparseArray<ArrayType>::FBaseIterator(*this, TBitArray::FBitIterator(AllocationFlags));
		}

		FORCEINLINE FSparseArrayElement& operator[](uint32 Index)
		{
			return *(FSparseArrayElement*)&Data[Index].ElementData;
		}
		FORCEINLINE const FSparseArrayElement& operator[](uint32 Index) const
		{
			return *(const FSparseArrayElement*)&Data[Index].ElementData;
		}
	};

	template<typename ElementType>
	class TSetElement
	{
	public:
		ElementType Value;
		mutable int32 HashNextId;
		mutable int32 HashIndex;

		FORCEINLINE bool operator==(const TSetElement& Other) const
		{
			return Value == Other.Value;
		}
		FORCEINLINE bool operator!=(const TSetElement& Other) const
		{
			return Value != Other.Value;
		}
	};
	
	template<typename SetType>
	class TSet
	{
	public:
		typedef TSetElement<SetType> ElementType;

	private:
		TSparseArray<ElementType> Elements;
		
		mutable TInlineAllocator<1>::ForElementType<int> Hash;
		mutable int32 HashSize;

	public:
		class FBaseIterator
		{
		private:
			TSet<SetType>& IteratedSet;
			TSparseArray<ElementType>::FBaseIterator ElementIt;

		public:
			FORCEINLINE FBaseIterator(const TSet<SetType>& InSet, TSparseArray<TSetElement<SetType>>::FBaseIterator InElementIt)
				: IteratedSet(const_cast<TSet<SetType>&>(InSet)), ElementIt(InElementIt)
			{
			}

			FORCEINLINE TSet<SetType>::FBaseIterator& operator++()
			{
				++ElementIt;
				return *this;
			}
			FORCEINLINE bool operator==(const TSet<SetType>::FBaseIterator& OtherIt) const
			{
				return ElementIt == OtherIt.ElementIt;
			}
			FORCEINLINE bool operator!=(const TSet<SetType>::FBaseIterator& OtherIt) const
			{
				return ElementIt != OtherIt.ElementIt;
			}
			FORCEINLINE TSet<SetType>::FBaseIterator& operator=(TSet<SetType>::FBaseIterator& OtherIt)
			{
				return ElementIt = OtherIt.ElementIt;
			}
			FORCEINLINE ElementType& operator*()
			{
				return *(ElementType*)&*ElementIt;
			}
			FORCEINLINE const ElementType& operator*() const
			{
				return *(ElementType*)&*ElementIt;
			}
			FORCEINLINE ElementType& operator->()
			{
				return *(ElementType*)&*ElementIt;
			}
			FORCEINLINE const ElementType& operator->() const
			{
				return *(ElementType*)&*ElementIt;
			}
		};

	public:
		FORCEINLINE TSet<SetType>::FBaseIterator begin()
		{
			return TSet<SetType>::FBaseIterator(*this, Elements.begin());
		}
		FORCEINLINE const TSet<SetType>::FBaseIterator begin() const
		{
			return TSet<SetType>::FBaseIterator(*this, Elements.begin());
		}
		FORCEINLINE TSet<SetType>::FBaseIterator end()
		{
			return TSet<SetType>::FBaseIterator(*this, Elements.end());
		}
		FORCEINLINE const TSet<SetType>::FBaseIterator end() const
		{
			return TSet<SetType>::FBaseIterator(*this, Elements.end());
		}
	};
	template<typename KeyType, typename ValueType>
	class TPair
	{
	private:
		KeyType First;
		ValueType Second;

		TPair(KeyType& Key, ValueType& Value)
			: First(Key), Second(Value)
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

	template<typename KeyType, typename ValueType>
	class TMap
	{
	public:
		typedef TPair<KeyType, ValueType> ElementType;

	private:
		TSet<ElementType> Pairs;

	public:
		class FBaseIterator
		{
		private:
			TMap<KeyType, ValueType>& IteratedMap;
			TSet<ElementType>::FBaseIterator SetIt;

		public:
			FBaseIterator(TMap<KeyType, ValueType>& InMap, TSet<ElementType>::FBaseIterator InSet)
				: IteratedMap(InMap), SetIt(InSet)
			{
			}
			FORCEINLINE TMap<KeyType, ValueType>::FBaseIterator operator++()
			{
				++SetIt;
				return *this;
			}
			FORCEINLINE TSet<ElementType>::ElementType& operator*()
			{
				return *SetIt;
			}
			FORCEINLINE const TSet<ElementType>::ElementType& operator*() const
			{
				return *SetIt;
			}
			FORCEINLINE bool operator==(const TMap<KeyType, ValueType>::FBaseIterator& Other) const
			{
				return SetIt == Other.SetIt;
			}
			FORCEINLINE bool operator!=(const TMap<KeyType, ValueType>::FBaseIterator& Other) const
			{
				return SetIt != Other.SetIt;
			}
		};

		FORCEINLINE TMap<KeyType, ValueType>::FBaseIterator begin()
		{
			return TMap<KeyType, ValueType>::FBaseIterator(*this, Pairs.begin());
		}
		FORCEINLINE const TMap<KeyType, ValueType>::FBaseIterator begin() const
		{
			return TMap<KeyType, ValueType>::FBaseIterator(*this, Pairs.begin());
		}
		FORCEINLINE TMap<KeyType, ValueType>::FBaseIterator end()
		{
			return TMap<KeyType, ValueType>::FBaseIterator(*this, Pairs.end());
		}
		FORCEINLINE const TMap<KeyType, ValueType>::FBaseIterator end() const
		{
			return TMap<KeyType, ValueType>::FBaseIterator(*this, Pairs.end());
		}
		FORCEINLINE ElementType& operator[](const KeyType& Key)
		{
			return this->GetByKey(Key);
		}
		FORCEINLINE const ElementType& operator[](const KeyType& Key) const
		{
			return this->GetByKey(Key);
		}

		template<typename ComparisonFunction>
		FORCEINLINE ElementType& GetByKey(const KeyType& Key, ComparisonFunction* comp = nullptr)
		{
			for (ElementType Pair : *this)
			{
				if (comp ? comp(Pair.First, Key) : Pair.Key() == Key)
				{
					return Pair;
				}
			}
		}

	};

	template<class ObjectType>
	class TSharedPtr
	{
	public:
		ObjectType* Object;

		int32_t SharedReferenceCount;
		int32_t WeakReferenceCount;
	};

	template<class ObjectType>
	class TSharedRef
	{
	private:
		struct Counts
		{
			int32_t SharedReferenceCount;
			int32_t WeakReferenceCount;
		};

	public:

		ObjectType* Object;
		Counts* ReferenceController;
	};

	template<class PtrType>
	class TWeakObjectPtr
	{
		int32 ObjectIndex;
		int32 ObjectSerialNumber;

		TWeakObjectPtr(const void* Object) // should be UObject, but I dont have a UObject here right now
		{
		}
	};


}