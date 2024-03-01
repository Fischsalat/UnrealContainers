#pragma once
#include <string>

namespace UC
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
		inline void* (*EngineRealloc)(void* Block, uint64 NewSize, uint32 Alignment) = nullptr;

		inline void Init(void* ReallocAddress)
		{
			if (EngineRealloc == nullptr) [[unlikely]]
				EngineRealloc = reinterpret_cast<decltype(EngineRealloc)>(ReallocAddress);
		}

		inline void* Malloc(uint64 Size, uint32 Alignment = 0x0 /* auto */)
		{
			return EngineRealloc(nullptr, Size, Alignment);
		}

		inline void* Realloc(void* Ptr, uint64 Size, uint32 Alignment = 0x0 /* auto */)
		{
			return EngineRealloc(Ptr, Size, Alignment);
		}

		inline void Free(void* Ptr)
		{
			EngineRealloc(Ptr, 0x0, 0x0);
		}
	}


	template<typename ArrayElementType>
	class TArray;

	template<typename SparseArrayElementType>
	class TSparseArray;

	template<typename SetElementType>
	class TSet;

	template<typename KeyElementType, typename ValueElementType>
	class TMap;

	template<typename KeyElementType, typename ValueElementType>
	class TPair;

	namespace Iterators
	{
		class FSetBitIterator;

	}


	namespace ContainerImpl
	{
		template<class ContainerType>
		class TCloneImpl
		{
		private:
			static constexpr int32 TypeSize = sizeof(ContainerType);
			static constexpr int32 TypeAlign = alignof(ContainerType);

		public:
			TCloneImpl() = delete;
			TCloneImpl& operator=(const TCloneImpl&) = delete;

		private:
			alignas(TypeAlign) uint8 ElementBuffer[TypeSize];

		public:
			inline operator ContainerType&&()
			{
				return reinterpret_cast<ContainerType&&>(ElementBuffer);
			}
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
				static constexpr int32 ElementSize = sizeof(ElementType);
				static constexpr int32 ElementAlign = alignof(ElementType);

			private:
				TAlignedBytes<ElementSize, ElementAlign> InlineData[NumInlineElements];
				ElementType* SecondaryData;

			public:
				ForElementType()
					: InlineData{ 0x0 }, SecondaryData(nullptr)
				{
				}

				ForElementType(ForElementType&& Other)
					: SecondaryData(Other.SecondaryData)
				{
					memcpy(InlineData, Other.InlineData, sizeof(InlineData));

					Other.SecondaryData = nullptr;
					/* Maybe set InlineData to zero */
				}

				~ForElementType()
				{
					Free();
				}

			public:
				ForElementType& operator=(ForElementType&& Other) noexcept
				{
					if (this == &Other)
						return *this;

					Free();

					SecondaryData = Other.SecondaryData;
					memcpy(InlineData, Other.InlineData, sizeof(InlineData));

					Other.SecondaryData = nullptr;
					/* Maybe set InlineData to zero */

					return *this;
				}

			public:
				inline void Free()
				{
					if (SecondaryData)
						FMemory::Free(SecondaryData);

					memset(InlineData, 0x0, sizeof(InlineData));
				}

			public:
				inline const ElementType* GetAllocation() const { return SecondaryData ? SecondaryData : reinterpret_cast<const ElementType*>(&InlineData); }

				inline uint32 GetNumInlineBytes() const { return NumInlineElements; }
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
			FBitArray()
				: NumBits(0), MaxBits(Data.GetNumInlineBytes() * NumBitsPerDWORD)
			{
			}

		public:
			FBitArray& operator=(FBitArray&&) = default;

			/* BitArrays should never actively be deep-copied with this operator */
			FBitArray& operator=(const FBitArray&) = delete;

		private:
			inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		public:
			inline int32 Num() const { return NumBits; }
			inline int32 Max() const { return MaxBits; }

			inline const uint32* GetData() const { return reinterpret_cast<const uint32*>(Data.GetAllocation()); }

			inline bool IsValidIndex(int32 Index) const { return Index > 0 && Index < NumBits; }

			inline bool IsValid() const { return GetData() && NumBits > 0; }

		public:
			inline bool operator[](int32 Index) const { VerifyIndex(Index); return GetData()[Index / NumBitsPerDWORD] & (1 << (Index & (NumBitsPerDWORD - 1))); }

			inline bool operator==(const FBitArray& Other) const { return NumBits == Other.NumBits && GetData() == Other.GetData(); }
			inline bool operator!=(const FBitArray& Other) const { return NumBits != Other.NumBits || GetData() != Other.GetData(); }

		public:
			//friend Iterators::FSetBitIterator begin(const FBitArray& Array);
			//friend Iterators::FSetBitIterator end(const FBitArray& Array);
		};

		template<typename SparseArrayType>
		union TSparseArrayElementOrFreeListLink
		{
			SparseArrayType ElementData;

			struct
			{
				int32 PrevFreeIndex;
				int32 NextFreeIndex;
			};
		};

		template<typename SetType>
		class SetElement
		{
		private:
			template<typename SetDataType>
			friend class TSet;

		private:
			SetType Value;
			int32 HashNextId;
			int32 HashIndex;
		};
	}


	template <typename KeyType, typename ValueType>
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
		inline       KeyType& Key()       { return First; }
		inline const KeyType& Key() const { return First; }

		inline       ValueType& Value()       { return Second; }
		inline const ValueType& Value() const { return Second; }
	};

	template<typename ArrayElementType>
	class TArray
	{
	protected:
		static constexpr uint32 ElementAlign = alignof(ArrayElementType);
		static constexpr uint32 ElementSize = sizeof(ArrayElementType);

	protected:
		ArrayElementType* Data;
		int32 NumElements;
		int32 MaxElements;

	public:
		TArray()
			: Data(nullptr), NumElements(0), MaxElements(0)
		{
		}

		TArray(uint32 Size)
			: Data(static_cast<ArrayElementType*>(FMemory::Malloc(Size * ElementSize, ElementAlign))), NumElements(0), MaxElements(Size)
		{
		}

		TArray(TArray&& Other)
			: Data(Other.Data), NumElements(Other.NumElements), MaxElements(Other.MaxElements)
		{
			Other.Data = nullptr;
			Other.NumElements = 0x0;
			Other.MaxElements = 0x0;
		}

		~TArray()
		{
			Free();
		}

	public:
		TArray& operator=(TArray&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Free();

			Data = Other.Data;
			NumElements = Other.NumElements;
			MaxElements = Other.MaxElements;

			Other.Data = nullptr;
			Other.NumElements = 0x0;
			Other.MaxElements = 0x0;

			return *this;
		}

	public:
		/* Use 'TArray<Type>& MyRef = OtherArray;' by default. If you want to copy this array use the 'Clone()' function */
		TArray(const TArray&) = delete;

		/* Use 'TArray<Type>& MyRef = OtherArray;' by default. If you want to copy this array use the 'Clone()' function */
		TArray& operator=(const TArray&) = delete;

	private:
		inline int32 GetSlack() const { return MaxElements - NumElements; }

		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

		inline       ArrayElementType& GetUnsafe(int32 Index) { return Data[Index]; }
		inline const ArrayElementType& GetUnsafe(int32 Index) const { return Data[Index]; }

	public:
		inline void Reserve(int32 Count)
		{
			if (GetSlack() < Count)
				MaxElements += Count; FMemory::Realloc(Data, MaxElements, ElementAlign);
		}

		inline void Add(const ArrayElementType& Element)
		{
			if (GetSlack() > 0)
				Reserve(3);

			Data[NumElements] = Element;
			NumElements++;
		}

		inline void CopyFrom(const TArray& Other)
		{
			if (this == &Other)
				return;

			NumElements = Other.NumElements;

			if (MaxElements >= Other.NumElements)
			{
				memcpy(Data, Other.Data, Other.NumElements);
				return;
			}

			FMemory::Realloc(Data, Other.NumElements, ElementAlign);
			MaxElements = Other.NumElements;
		}

		inline void Remove(int32 Index)
		{
			if (!IsValidIndex(Index))
				return;

			NumElements--;

			for (int i = Index; i < NumElements; i++)
			{
				/* NumElements was decremented, acessing i + 1 is safe */
				Data[i] = Data[i + 1];
			}
		}

		inline void Clear()
		{
			if (!Data)
				return;

			memset(Data, 0, NumElements * ElementSize);
			NumElements = 0;
		}

		inline void Free() noexcept
		{
			if (Data)
				FMemory::Free(Data);

			NumElements = 0x0;
			MaxElements = 0x0;
		}

	public:
		inline ContainerImpl::TCloneImpl<TArray> Clone() const
		{
			TArray NewArray;
			NewArray.CopyFrom(*this);

			return *reinterpret_cast<ContainerImpl::TCloneImpl<TArray>*>(&NewArray);
		}

	public:
		inline int32 Num() const { return NumElements; }
		inline int32 Max() const { return NumElements; }

		inline bool IsValidIndex(int32 Index) const { return Data && Index >= 0 && Index < NumElements; }

		inline bool IsValid() const { return Data && NumElements > 0 && MaxElements > 0; }

	public:
		inline       ArrayElementType& operator[](int32 Index)       { VerifyIndex(Index); return Data[Index]; }
		inline const ArrayElementType& operator[](int32 Index) const { VerifyIndex(Index); return Data[Index]; }

		inline bool operator==(const TArray<ArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TArray<ArrayElementType>& Other) const { return Data != Other.Data; }

		inline explicit operator bool() const { return IsValid(); };

	public:
		//template<typename T> friend Iterators::TArrayIterator<T> begin(const TArray& Array);
		//template<typename T> friend Iterators::TArrayIterator<T> end(const TArray& Array);
	};

	class FString : public TArray<wchar_t>
	{
	public:
		using TArray::TArray;

		FString(const wchar_t* Str)
			: TArray(static_cast<uint32>(wcslen(Str) + sizeof(L'\0')))
		{
			const uint64 NullTerminatedLength = wcslen(Str) + sizeof(L'\0');

			memcpy(Data, Str, NullTerminatedLength);
		}

	public:
		inline ContainerImpl::TCloneImpl<FString> Clone() const
		{
			FString NewString;
			NewString.CopyFrom(*this);

			return *reinterpret_cast<ContainerImpl::TCloneImpl<FString>*>(&NewString);
		}

	public:
		inline bool operator==(const FString& Other) const { return Other ? NumElements == Other.NumElements && wcscmp(Data, Other.Data) == 0 : false; }
		inline bool operator!=(const FString& Other) const { return Other ? NumElements != Other.NumElements || wcscmp(Data, Other.Data) != 0 : true; }
	};

	template<typename SparseArrayElementType>
	class TSparseArray
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SparseArrayElementType);
		static constexpr uint32 ElementSize = sizeof(SparseArrayElementType);

	private:
		using FElementOrFreeListLink = ContainerImpl::TSparseArrayElementOrFreeListLink<ContainerImpl::TAlignedBytes<ElementSize, ElementAlign>>;

	private:
		TArray<FElementOrFreeListLink> Data;
		ContainerImpl::FBitArray AllocationFlags;
		int32 FirstFreeIndex;
		int32 NumFreeIndices;

	public:
		TSparseArray()
			: FirstFreeIndex(-1), NumFreeIndices(0)
		{
		}

		TSparseArray(TSparseArray&& Other) = default;

	public:
		TSparseArray& operator=(TSparseArray&&) = default;

		/* Use 'TSparseArray<Type>& MyRef = OtherArray;' by default. If you want to copy this array use the 'Clone()' function */
		TSparseArray& operator=(const TSparseArray&) = delete;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 Num() const { return Data.Num() - NumFreeIndices; }
		inline int32 Max() const { return Data.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Data.IsValidIndex(Index) && AllocationFlags[Index]; }

		inline bool IsValid() const { return Data.IsValid() && AllocationFlags.IsValid(); }

	public:
		inline       SparseArrayElementType& operator[](int32 Index)       { VerifyIndex(Index); return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData); }
		inline const SparseArrayElementType& operator[](int32 Index) const { VerifyIndex(Index); return *reinterpret_cast<SparseArrayElementType*>(&Data.GetUnsafe(Index).ElementData); }

		inline bool operator==(const TSparseArray<SparseArrayElementType>& Other) const { return Data == Other.Data; }
		inline bool operator!=(const TSparseArray<SparseArrayElementType>& Other) const { return Data != Other.Data; }

	public:
		//template<typename T> friend Iterators::TSparseArrayIterator<T> begin(const TSparseArray& Array);
		//template<typename T> friend Iterators::TSparseArrayIterator<T> end(const TSparseArray& Array);
	};

	template<typename SetElementType>
	class TSet
	{
	private:
		static constexpr uint32 ElementAlign = alignof(SetElementType);
		static constexpr uint32 ElementSize = sizeof(SetElementType);

	private:
		using SetDataType = ContainerImpl::SetElement<SetElementType>;
		using HashType = ContainerImpl::TInlineAllocator<1>::ForElementType<int32>;

	private:
		TSparseArray<SetDataType> Elements;
		HashType Hash;
		int32 HashSize;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		inline       SetDataType& operator[] (int32 Index)       { return Elements[Index].Value; }
		inline const SetDataType& operator[] (int32 Index) const { return Elements[Index].Value; }

		inline bool operator==(const TSet<SetDataType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TSet<SetDataType>& Other) const { return Elements != Other.Elements; }

	public:
		//template<typename T> friend Iterators::TSetIterator<T> begin(const TSet& Set);
		//template<typename T> friend Iterators::TSetIterator<T> end(const TSet& Set);
	};

	template<typename KeyElementType, typename ValueElementType>
	class TMap
	{
	public:
		using ElementType = TPair<KeyElementType, ValueElementType>;

	private:
		TSet<ElementType> Elements;

	private:
		inline void VerifyIndex(int32 Index) const { if (!IsValidIndex(Index)) throw std::out_of_range("Index was out of range!"); }

	public:
		inline int32 Num() const { return Elements.Num(); }
		inline int32 Max() const { return Elements.Max(); }

		inline bool IsValidIndex(int32 Index) const { return Elements.IsValidIndex(Index); }

		inline bool IsValid() const { return Elements.IsValid(); }

	public:
		//inline decltype(auto) Find(const KeyElementType& Key, bool(*Equals)(const KeyElementType& Key, const ValueElementType& Value))
		//{
		//	for (auto It = begin(*this); It != end(*this); ++It)
		//	{
		//		if (Equals(It->Key(), Key))
		//			return It;
		//	}
		//
		//	return end(*this);
		//}

	public:
		inline       ElementType& operator[] (int32 Index)       { return Elements[Index]; }
		inline const ElementType& operator[] (int32 Index) const { return Elements[Index]; }

		inline bool operator==(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements == Other.Elements; }
		inline bool operator!=(const TMap<KeyElementType, ValueElementType>& Other) const { return Elements != Other.Elements; }

	public:
		//template<typename T0, typename T1> friend Iterators::TMapIterator<T0, T1> begin(const TMap& Map);
		//template<typename T0, typename T1> friend Iterators::TMapIterator<T0, T1> end(const TMap& Map);
	};
}