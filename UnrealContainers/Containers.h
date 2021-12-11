#pragma once

namespace UE
{
	// FMemory::Malloc: Allows for initialization of TSets and TMaps that aren't already initialized
	// FMemory::Realloc: Allows for reallocation of Container data, used to expand TArrays and TMaps
	// FMemory::Free: Allows to free TArrays and FStrings allocated by the engine (eg. FName::ToString)

	// Why not just use standard C free and realloc? You can't play with heap memory allocated by another program, it'll crash

	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	/* Add a pattern or offset here (reason above) */
	static auto Malloc = reinterpret_cast<void* (*)(int32 Size, int32 Alignment)>(uintptr_t(GetModuleHandle(0)) + 0x10B54D0);
	static auto Realloc = reinterpret_cast<void* (*)(void* Memory, int64 NewSize, uint32 Alignment)>(uintptr_t(GetModuleHandle(0)) + 0x10B7900);
	static auto Free = reinterpret_cast<void (*)(void* Memory)>(uintptr_t(GetModuleHandle(0)) + 0x10AC0F0);


	template<class TArrayType>
	class TArray
	{
		friend class FString;
		template<typename ArrayType> friend class TSparseArray;
		template<typename SetType>   friend class TSet;

	public:
		FORCEINLINE int Num() const
		{
			return Count;
		}
		FORCEINLINE int Max() const
		{
			return MaxElements;
		}
		FORCEINLINE int Slack() const
		{
			return MaxElements - Count;
		}
		FORCEINLINE void Reserve(const int NumElements)
		{
			Data = Slack() >= NumElements ? Data : (TArrayType*)Realloc(Data, (MaxElements = Count + NumElements) * sizeof(TArrayType), 0);
		}
		FORCEINLINE void Reset(int MinSizeAfterReset = 0)
		{
			if (MaxElements >= MinSizeAfterReset)
			{
				Count = 0;

			}
		}
		FORCEINLINE void RemoveAt(const int Index, const int Lenght)
		{
			
		}
		FORCEINLINE void Add(TArrayType InData...)
		{
			int Num = sizeof(InData) / sizeof(TArrayType);

			Reserve(Num);
			Data[Count] = InData;
			Count += Num;
		};
		FORCEINLINE void FreeArray()
		{
			Free(Data);
			Data = nullptr;
			Count = 0;
			MaxElements = 0;
		}

		FORCEINLINE TArrayType& operator[](int i)
		{
			return Data[i];
		};

		FORCEINLINE const TArrayType& operator[](int i) const
		{
			return Data[i];
		};

		FORCEINLINE int MaxIndex()
		{
			return Count - 1;
		}

		class FBaseArrayIterator
		{
			int32 Index;
			TArray<TArrayType>& ItArray;

		public:
			FBaseArrayIterator(TArray<TArrayType>& Array)
				: ItArray(Array), Index(Array.Count)
			{
			}
			FBaseArrayIterator(TArray<TArrayType>& Array, int32 CurrentIndex)
				: ItArray(Array), Index(CurrentIndex)
			{
			}

			FORCEINLINE TArrayType& operator*()
			{
				return ItArray[Index];
			}
			FORCEINLINE TArrayType& operator->()
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

		FORCEINLINE FBaseArrayIterator begin()
		{
			return FBaseArrayIterator(*this, 0);
		}
		FORCEINLINE FBaseArrayIterator begin() const
		{
			return FBaseArrayIterator(*this, 0);
		}
		FORCEINLINE FBaseArrayIterator end()
		{
			return FBaseArrayIterator(*this);
		}
		FORCEINLINE FBaseArrayIterator end() const
		{
			return FBaseArrayIterator(*this);
		}

	private:
		TArrayType* Data;
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

		FORCEINLINE bool IsValid() const
		{
			return Data != nullptr;
		}

		FORCEINLINE const wchar_t* ToWString() const
		{
			return Data;
		}
		FORCEINLINE void Free()
		{
			this->FreeArray();
		}

		FORCEINLINE std::string ToString() const
		{
			auto length = wcslen(Data);

			std::string str(length, '\0');

			std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

			return str;
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

			FORCEINLINE void operator=(void* InElements)
			{
				SecondaryData = InElements;
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
		template<typename ArrayType> friend class TSparseArray;
		template<typename SetType>   friend class TSet;

	private:
		TInlineAllocator<4>::ForElementType<uint32> Data;
		int32 NumBits;
		int32 MaxBits;

		struct FRelativeBitReference
		{
		public:
			FORCEINLINE explicit FRelativeBitReference(int32 BitIndex)
				: DWORDIndex(BitIndex >> ((int32)5)), Mask(1 << (BitIndex & (((int32)32) - 1)))
			{
			}

			int32  DWORDIndex;
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

			FORCEINLINE void SetBit(const bool Value)
			{
				Value ? Data |= Mask : Data &= ~Mask;

				// 10011101 - Data			 // 10011101 - Data
				// 00000010 - Mask - true |	 // 00000010 - Mask - false
				// 10011111	-  |=			 // 11111101 -  ~
				//							 // 10011111 -  &=
			}

			FORCEINLINE operator bool() const
			{
				return (Data & Mask) != 0;
			}
			FORCEINLINE void operator=(const bool Value)
			{
				this->SetBit(Value);
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
		FORCEINLINE FBitIterator Iterator(int32 StartIndex)
		{
			return FBitIterator(*this, StartIndex);
		}

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
		FORCEINLINE void Set(const int32 Index, const bool Value, bool bIsSettingAllZero = false)
		{
			const int32 DWORDIndex = (Index >> ((int32)5));
			const int32 Mask = (1 << (Index & (((int32)32) - 1)));

			if(!bIsSettingAllZero)
				NumBits = Index >= NumBits ? Index < MaxBits ? Index + 1 : NumBits : NumBits;

			FBitReference(Data[DWORDIndex], Mask).SetBit(Value);
		}
		FORCEINLINE void ZeroAll()
		{
			for (int i = 0; i < MaxBits; i++)
			{
				Set(i, false, true);
			}
		}
	};

	template<typename ElementType>
	union TSparseArrayElementOrListLink
	{
		TSparseArrayElementOrListLink(ElementType& InElement)
			: ElementData(InElement)
		{
		}
		TSparseArrayElementOrListLink(ElementType&& InElement)
			: ElementData(InElement)
		{
		}

		TSparseArrayElementOrListLink(int32 InPrevFree, int32 InNextFree)
			: PrevFreeIndex(InPrevFree), NextFreeIndex(InNextFree)
		{
		}

		TSparseArrayElementOrListLink<ElementType> operator=(const TSparseArrayElementOrListLink<ElementType>& Other)
		{
			return TSparseArrayElementOrListLink(Other.NextFreeIndex, Other.PrevFreeIndex);
		}

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
	private:
		template<typename SetType> friend class TSet;

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

		FORCEINLINE int32 GetNumFreeIndices() const
		{
			return NumFreeIndices;
		}
		FORCEINLINE int32 GetFirstFreeIndex() const
		{
			return FirstFreeIndex;
		}
		FORCEINLINE const TArray<FSparseArrayElement>& GetData() const
		{
			return Data;
		}
		FORCEINLINE const TBitArray& GetAllocationFlags() const
		{
			return AllocationFlags;
		}
		FORCEINLINE int32 AddSingle(ArrayType InElement)
		{
			FSparseArrayElement Element(InElement);

			int32 NextFree;
			int32 OutIndex;
			if (FirstFreeIndex >= 1)
			{
				NextFree = Data[FirstFreeIndex].NextFreeIndex;
				Data[FirstFreeIndex] = Element;
				--NumFreeIndices;

				AllocationFlags.Set(FirstFreeIndex, true);

				if (NumFreeIndices >= 1)
				{
					OutIndex = NextFree;
					FirstFreeIndex = NextFree;
					Data[NextFree].PrevFreeIndex = -1;

					return OutIndex;
				}
			}
			else
			{
				Data.Add(Element);
				AllocationFlags.Set(Data.Num() - 1, true);

				return Data.Num() - 1;
			}
		}
		/*
		FORCEINLINE void Add(ArrayType... Elements)
		{
			va_list myList;

			va_start(myList, Elements);

			const int32 Num = sizeof(Elements) / sizeof(ArrayType);

			for (int i = 0; i < Num; ++i)
			{
			//	this->AddSinge(va_arg(myList, Elements));
			}

			va_end(myList);
		}
		*/
	};

	template<typename ElementType>
	class TSetElement
	{
	public:
		ElementType Value;
		mutable int32 HashNextId;
		mutable int32 HashIndex;

		TSetElement(ElementType InValue, int32 InHashNextId, int32 InHashIndex)
			: Value(InValue), HashNextId(InHashNextId), HashIndex(InHashIndex)
		{
		}

		FORCEINLINE TSetElement<ElementType>& operator=(const TSetElement<ElementType>& Other)
		{
			Value = Other.Value;

		}

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
	private:
		friend TSparseArray;

	public:
		typedef TSetElement<SetType> ElementType;
		typedef TSparseArrayElementOrListLink<ElementType> ArrayElementType;

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
			FORCEINLINE SetType& operator*()
			{
				return *ElementIt.Value;
			}
			FORCEINLINE const SetType& operator*() const
			{
				return *ElementIt.Value;
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

		FORCEINLINE bool IsValid() const
		{
			return Elements.Data.Data != nullptr && Elements.AllocationFlags.MaxBits > 0;
		}
		FORCEINLINE TSparseArray<ElementType>& GetElements()
		{
			return Elements;
		}
		FORCEINLINE const TSparseArray<ElementType>& GetElements() const
		{
			return Elements;
		}

		FORCEINLINE int32 AddSingle(SetType InElement, int32 InHashIndex = 0, int32 InHashNextId = 0)
		{
			//ToDo: Check for duplication

			if (!this->IsValid())
				return this->Initialize();

			return Elements.AddSingle({ InElement, InHashIndex, InHashNextId });
		}
		FORCEINLINE void Initialize(const int32 NumElementsToInitWith = 5)
		{
			if (this->IsValid())
				return;

			Elements.Data.MaxElements = NumElementsToInitWith;
			Elements.Data.Count = NumElementsToInitWith;
			Elements.Data.Data = (ArrayElementType*)(Malloc(NumElementsToInitWith * sizeof(ElementType), alignof(ElementType)));
			for (int i = 0; i < NumElementsToInitWith; i++)
			{
				Elements.Data.Data[i].PrevFreeIndex = i - 1;
				Elements.Data.Data[i].NextFreeIndex = i + 1;
			}

			Elements.FirstFreeIndex = 0;
			Elements.NumFreeIndices = NumElementsToInitWith;

			Elements.AllocationFlags.MaxBits = 128;
			Elements.AllocationFlags.NumBits = NumElementsToInitWith;
			Elements.AllocationFlags.ZeroAll();

			Hash = Malloc(NumElementsToInitWith * sizeof(ElementType), alignof(ElementType));
			HashSize = NumElementsToInitWith * sizeof(ElementType);
		}
	};
	template<typename KeyType, typename ValueType>
	class TPair
	{
	private:
		KeyType First;
		ValueType Second;

	public:
		TPair(KeyType Key, ValueType Value)
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

		FORCEINLINE int32 AddSingle(KeyType InKey, ValueType InValue)
		{
			return Pairs.AddSingle({ InKey, InValue });
		}
		FORCEINLINE int32 AddSingle(ElementType InElement)
		{
			return Pairs.AddSingle(InElement);
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
			return nullptr;
		}

		FORCEINLINE bool IsValid() const
		{
			return Pairs.IsValid();
		}
		FORCEINLINE void Initialize(const int32 NumElementsToInitWith = 5)
		{
			return Pairs.Initialize(NumElementsToInitWith);
		}

	};

	template<class ObjectType>
	class TSharedPtr
	{
	public:
		ObjectType* Object;

		int32 SharedReferenceCount;
		int32 WeakReferenceCount;
	};

	template<class ObjectType>
	class TSharedRef
	{
	private:
		struct Counts
		{
			int32 SharedReferenceCount;
			int32 WeakReferenceCount;
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