#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

/* Credits: https://en.cppreference.com/w/cpp/string/byte/tolower */
inline std::string str_tolower(std::string S)
{
	std::transform(S.begin(), S.end(), S.begin(), [](unsigned char C) { return std::tolower(C); });
	return S;
}

namespace ASMUtils
{
	inline bool IsRIPRelativeJump(uintptr_t Address)
	{
		return *reinterpret_cast<uint16_t*>(Address) == 0xFF48; /* 48 for jmp, FF for "RIP relative" -- little endian */
	}

	inline uintptr_t Resolve32BitRIPRelativeJump(uintptr_t Address)
	{
		/*
		* 48 FF 25 C1 10 06 00     jmp QWORD [rip+0x610c1]
		* 
		* 48 FF 25 <-- Information on the instruction [jump, relative, rip]
		* C1 10 06 00 <-- 32-bit Offset relative to the address coming **after** these instructions (+ 7) [if 48 had hte address 0x0 the offset would be relative to address 0x7]
		*/

		return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
	}

	inline uintptr_t Resolve32BitSectionRelativeCall(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a call, with one less instruction byte */
		return ((Address + 6) + *reinterpret_cast<int32_t*>(Address + 2));
	}

	inline uintptr_t Resolve32BitRelativeCall(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a non-relative call, with two less instruction byte */
		return ((Address + 5) + *reinterpret_cast<int32_t*>(Address + 1));
	}

	inline uintptr_t Resolve32BitRelativeMove(uintptr_t Address)
	{
		/* Same as in Resolve32BitRIPRelativeJump, but instead of a jump we resolve a relative mov */
		return ((Address + 7) + *reinterpret_cast<int32_t*>(Address + 3));
	}
}

struct CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
};

struct TEB
{
	NT_TIB NtTib;
	PVOID EnvironmentPointer;
	CLIENT_ID ClientId;
	PVOID ActiveRpcHandle;
	PVOID ThreadLocalStoragePointer;
	struct PEB* ProcessEnvironmentBlock;
};

struct PEB_LDR_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	BYTE MoreFunnyPadding[0x3];
	HANDLE SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID EntryInProgress;
	BOOLEAN ShutdownInProgress;
	BYTE MoreFunnyPadding2[0x7];
	HANDLE ShutdownThreadId;
};

struct PEB
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN SpareBits : 1;
		};
	};
	BYTE ManuallyAddedPaddingCauseTheCompilerIsStupid[0x4]; // It doesn't 0x8 byte align the pointers properly 
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PEB_LDR_DATA* Ldr;
};

struct UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	BYTE MoreStupidCompilerPaddingYay[0x4];
	PWCH Buffer;
};

struct LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	//union
	//{
	//	LIST_ENTRY InInitializationOrderLinks;
	//	LIST_ENTRY InProgressLinks;
	//};
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	BYTE MoreStupidCompilerPaddingYay[0x4];
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
}; 

inline _TEB* _NtCurrentTeb()
{
	return reinterpret_cast<struct _TEB*>(__readgsqword(((LONG)__builtin_offsetof(NT_TIB, Self))));
}

inline PEB* GetPEB()
{
	return reinterpret_cast<TEB*>(_NtCurrentTeb())->ProcessEnvironmentBlock;
}

inline uintptr_t GetImageBase()
{
	return reinterpret_cast<uintptr_t>(GetPEB()->ImageBaseAddress);
}

inline uintptr_t GetOffset(void* Addr)
{
	static uintptr_t ImageBase = 0x0;

	if (ImageBase == 0x0)
		ImageBase = GetImageBase();

	uintptr_t AddrAsInt = reinterpret_cast<uintptr_t>(Addr);

	return AddrAsInt > ImageBase ? (AddrAsInt - ImageBase) : 0x0;
}

inline bool IsInProcessRange(uintptr_t Address)
{
	uintptr_t ImageBase = GetImageBase();
	PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase)->e_lfanew);

	return Address > ImageBase && Address < (NtHeader->OptionalHeader.SizeOfImage + ImageBase);
}

inline bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi;

	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		constexpr DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			b = true;

		return b;
	}

	return true;
};

inline LDR_DATA_TABLE_ENTRY* GetModuleLdrTableEntry(const char* SearchModuleName)
{
	PEB* Peb = GetPEB();
	PEB_LDR_DATA* Ldr = Peb->Ldr;

	int NumEntriesLeft = Ldr->Length;

	for (LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
	{
		LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(P);

		std::wstring WideModuleName(Entry->BaseDllName.Buffer, Entry->BaseDllName.Length >> 1);
		std::string ModuleName = std::string(WideModuleName.begin(), WideModuleName.end());

		if (str_tolower(ModuleName) == str_tolower(SearchModuleName))
			return Entry;
	}

	return nullptr;
}

inline void* GetModuleAddress(const char* SearchModuleName)
{
	LDR_DATA_TABLE_ENTRY* Entry = GetModuleLdrTableEntry(SearchModuleName);

	if (Entry)
		return Entry->DllBase;

	return nullptr;
}

/* Gets the address at which a pointer to an imported function is stored */
inline PIMAGE_THUNK_DATA GetImportAddress(uintptr_t ModuleBase, const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	/* Get the module importing the function */
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

	if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

	if (!NtHeader)
		return nullptr;

	PIMAGE_IMPORT_DESCRIPTOR ImportTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	//std::cout << "ModuleName: " << (SearchModuleName ? SearchModuleName : "Default") << std::endl;

	/* Loop all modules and if we found the right one, loop all imports to get the one we need */
	for (PIMAGE_IMPORT_DESCRIPTOR Import = ImportTable; Import && Import->Characteristics != 0x0; Import++)
	{
		if (Import->Name == 0xFFFF)
			continue;

		const char* Name = reinterpret_cast<const char*>(ModuleBase + Import->Name);

		//std::cout << "Name: " << str_tolower(Name) << std::endl;

		if (str_tolower(Name) != str_tolower(ModuleToImportFrom))
			continue;

		PIMAGE_THUNK_DATA NameThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(ModuleBase + Import->OriginalFirstThunk);
		PIMAGE_THUNK_DATA FuncThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(ModuleBase + Import->FirstThunk);

		while (!IsBadReadPtr(reinterpret_cast<void*>(NameThunk))
			&& !IsBadReadPtr(reinterpret_cast<void*>(FuncThunk))
			&& !IsBadReadPtr(reinterpret_cast<void*>(ModuleBase + NameThunk->u1.AddressOfData))
			&& !IsBadReadPtr(reinterpret_cast<void*>(FuncThunk->u1.AddressOfData)))
		{
			/*
			* A functin might be imported using the Ordinal (Index) of this function in the modules export-table
			*
			* The name could probably be retrieved by looking up this Ordinal in the Modules export-name-table
			*/
			if ((NameThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) != 0) // No ordinal
			{
				NameThunk++;
				FuncThunk++;
				continue; // Maybe Handle this in the future
			}

			/* Get Import data for this function */
			PIMAGE_IMPORT_BY_NAME NameData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(ModuleBase + NameThunk->u1.ForwarderString);
			PIMAGE_IMPORT_BY_NAME FunctionData = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(FuncThunk->u1.AddressOfData);

			//std::cout << "IMPORT: " << std::string(NameData->Name) << std::endl;

			if (std::string(NameData->Name) == SearchFunctionName)
				return FuncThunk;

			NameThunk++;
			FuncThunk++;
		}
	}

	return nullptr;
}

/* Gets the address at which a pointer to an imported function is stored */
inline PIMAGE_THUNK_DATA GetImportAddress(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	const uintptr_t SearchModule = SearchModuleName ? reinterpret_cast<uintptr_t>(GetModuleAddress(SearchModuleName)) : GetImageBase();

	return GetImportAddress(SearchModule, ModuleToImportFrom, SearchFunctionName);
}

/* Finds the import for a funciton and returns the address of the function from the imported module */
inline void* GetAddressOfImportedFunction(const char* SearchModuleName, const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	PIMAGE_THUNK_DATA FuncThunk = GetImportAddress(SearchModuleName, ModuleToImportFrom, SearchFunctionName);

	if (!FuncThunk)
		return nullptr;

	return reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(FuncThunk->u1.AddressOfData);
}

inline void* GetAddressOfImportedFunctionFromAnyModule(const char* ModuleToImportFrom, const char* SearchFunctionName)
{
	PEB* Peb = GetPEB();
	PEB_LDR_DATA* Ldr = Peb->Ldr;

	int NumEntriesLeft = Ldr->Length;

	for (LIST_ENTRY* P = Ldr->InMemoryOrderModuleList.Flink; P && NumEntriesLeft-- > 0; P = P->Flink)
	{
		LDR_DATA_TABLE_ENTRY* Entry = reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(P);

		PIMAGE_THUNK_DATA Import = GetImportAddress(reinterpret_cast<uintptr_t>(Entry->DllBase), ModuleToImportFrom, SearchFunctionName);

		if (Import)
			return reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(Import->u1.AddressOfData);
	}

	return nullptr;
}

/* Gets the address of an exported function */
inline void* GetExportAddress(const char* SearchModuleName, const char* SearchFunctionName)
{
	/* Get the module the function was exported from */
	uintptr_t ModuleBase = reinterpret_cast<uintptr_t>(GetModuleAddress(SearchModuleName));
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

	if (ModuleBase == 0x0 || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(ModuleBase + reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase)->e_lfanew);

	if (!NtHeader)
		return nullptr;

	/* Get the table of functions exported by the module */
	PIMAGE_EXPORT_DIRECTORY ExportTable = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ModuleBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	const DWORD* NameOffsets = reinterpret_cast<const DWORD*>(ModuleBase + ExportTable->AddressOfNames);
	const DWORD* FunctionOffsets = reinterpret_cast<const DWORD*>(ModuleBase + ExportTable->AddressOfFunctions);

	const WORD* Ordinals = reinterpret_cast<const WORD*>(ModuleBase + ExportTable->AddressOfNameOrdinals);

	/* Iterate all names and return the function if the name matches what we're looking for */
	for (int i = 0; i < ExportTable->NumberOfFunctions; i++)
	{
		const WORD NameIndex = Ordinals[i];
		const char* Name = reinterpret_cast<const char*>(ModuleBase + NameOffsets[NameIndex]);

		if (strcmp(SearchFunctionName, Name) == 0)
			return reinterpret_cast<void*>(ModuleBase + FunctionOffsets[i]);
	}

	return nullptr;
}

inline void* FindPatternInRange(std::vector<int>&& Signature, uint8_t* Start, uintptr_t Range, bool bRelative = false, uint32_t Offset = 0, int SkipCount = 0)
{
	const auto PatternLength = Signature.size();
	const auto PatternBytes = Signature.data();

	for (int i = 0; i < (Range - PatternLength); i++)
	{
		bool bFound = true;
		int CurrentSkips = 0;

		for (auto j = 0ul; j < PatternLength; ++j)
		{
			if (Start[i + j] != PatternBytes[j] && PatternBytes[j] != -1)
			{
				bFound = false;
				break;
			}
		}
		if (bFound)
		{
			if (CurrentSkips != SkipCount)
			{
				CurrentSkips++;
				continue;
			}

			uintptr_t Address = uintptr_t(Start + i);
			if (bRelative)
			{
				if (Offset == -1)
					Offset = PatternLength;

				Address = ((Address + Offset + 4) + *(int32_t*)(Address + Offset));
			}
			return (void*)Address;
		}
	}

	return nullptr;
}

inline void* FindPatternInRange(const char* Signature, uint8_t* Start, uintptr_t Range, bool bRelative = false, uint32_t Offset = 0)
{
	static auto patternToByte = [](const char* pattern) -> std::vector<int>
	{
		auto Bytes = std::vector<int>{};
		const auto Start = const_cast<char*>(pattern);
		const auto End = const_cast<char*>(pattern) + strlen(pattern);

		for (auto Current = Start; Current < End; ++Current)
		{
			if (*Current == '?')
			{
				++Current;
				if (*Current == '?') ++Current;
				Bytes.push_back(-1);
			}
			else { Bytes.push_back(strtoul(Current, &Current, 16)); }
		}
		return Bytes;
	};

	return FindPatternInRange(patternToByte(Signature), Start, Range, bRelative, Offset);
}

inline void* FindPattern(const char* Signature, uint32_t Offset = 0, bool bSearchAllSegments = false, uintptr_t StartAddress = 0x0)
{
	uintptr_t ImageBase = GetImageBase();

	const PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase);
	const PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + DosHeader->e_lfanew);

	const DWORD SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;

	uintptr_t SearchStart = ImageBase;
	uintptr_t SearchRange = SizeOfImage;

	if (!bSearchAllSegments)
	{
		PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeaders);

		uintptr_t TextSection = 0x0;
		DWORD TextSize = 0;

		for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
		{
			IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

			std::string SectionName = (const char*)CurrentSection.Name;

			if (SectionName == ".text" && !TextSection)
			{
				TextSection = (ImageBase + CurrentSection.VirtualAddress);
				TextSize = CurrentSection.Misc.VirtualSize;
			}
		}

		SearchStart = TextSection;
		SearchRange = TextSize;
	}

	const uintptr_t SearchEnd = ImageBase + SearchRange;

	/* If the StartAddress is not default nullptr, and is out of memory-range */
	if (StartAddress != 0x0 && (StartAddress < SearchStart || StartAddress > SearchEnd))
		return nullptr;

	/* Add a byte to the StartAddress to prevent instantly returning the previous result */
	SearchStart = StartAddress != 0x0 ? (StartAddress + 0x1) : ImageBase;
	SearchRange = StartAddress != 0x0 ? SearchEnd - StartAddress : SizeOfImage;

	if (StartAddress == 0x0)
		StartAddress = SearchStart;

	return FindPatternInRange(Signature, reinterpret_cast<uint8_t*>(StartAddress), SearchRange, Offset != 0x0, Offset);
}


template<typename T>
inline T* FindAlignedValueInProcessInRange(T Value, int32_t Alignment, uintptr_t StartAddress, uint32_t Range)
{
	constexpr int32_t ElementSize = sizeof(T);

	for (uint32_t i = 0x0; i < Range; i += Alignment)
	{
		T* TypedPtr = reinterpret_cast<T*>(StartAddress + i);

		if (*TypedPtr == Value)
			return TypedPtr;
	}

	return nullptr;
}

template<typename T>
inline T* FindAlignedValueInProcess(T Value, const std::string& Sectionname = ".data", int32_t Alignment = alignof(T), bool bSearchAllSegments = false)
{
	uint8_t* ImageBase = (uint8_t*)GetImageBase();

	const auto DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	const auto NtHeaders = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);

	const DWORD SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;

	uint8_t* SearchStart = ImageBase;
	DWORD SearchSize = SizeOfImage;

	if (!bSearchAllSegments)
	{
		PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeaders);

		for (int i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
		{
			IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

			std::string SectionName = (const char*)CurrentSection.Name;

			if (SectionName == Sectionname)
			{
				SearchStart = (ImageBase + CurrentSection.VirtualAddress);
				SearchSize = CurrentSection.Misc.VirtualSize;
				break;
			}
		}
	}

	T* Result = FindAlignedValueInProcessInRange(Value, Alignment, reinterpret_cast<uintptr_t>(SearchStart), SearchSize);

	if (!Result && SearchStart != ImageBase)
		return FindAlignedValueInProcess(Value, Sectionname, Alignment, true);

	return Result;
}

struct MemAddress
{
public:
	uint8_t* Address;

private:
	//pasted
	static std::vector<int32_t> PatternToBytes(const char* pattern)
	{
		auto bytes = std::vector<int>{};
		const auto start = const_cast<char*>(pattern);
		const auto end = const_cast<char*>(pattern) + strlen(pattern);

		for (auto current = start; current < end; ++current)
		{
			if (*current == '?')
			{
				++current;
				if (*current == '?')
					++current;
				bytes.push_back(-1);
			}
			else { bytes.push_back(strtoul(current, &current, 16)); }
		}
		return bytes;
	}

	static bool IsFunctionRet(uint8_t* Address)
	{
		int Align = 0x10 - (uintptr_t(Address) % 0x10);
		//if (Opcode == RET && (OpcodeBefore is a POP opcode || OpcodeTwoBefore is a different POP Opcode)
		return Address[0] == 0xC3 && Address[Align] == 0x40 && ((Address[-1] >= 0x58 && Address[-1] <= 0x5F) || (Address[-2] == 0x41 && (Address[-1] >= 0x58 && Address[-1] <= 0x5F)));
	}

public:
	inline MemAddress(void* Addr)
		: Address((uint8_t*)Addr)
	{
	}
	inline MemAddress(uintptr_t Addr)
		: Address((uint8_t*)Addr)
	{
	}
	explicit operator bool()
	{
		return Address != nullptr;
	}
	explicit operator uint8_t*()
	{
		return Address;
	}
	operator void*()
	{
		return Address;
	}
	operator uintptr_t()
	{
		return uintptr_t(Address);
	}

	inline MemAddress operator+(int Value) const
	{
		return Address + Value;
	}

	template<typename T = void>
	inline T* Get()
	{
		return Address;
	}

	MemAddress FindFunctionEnd()
	{
		if (!Address)
			return MemAddress(nullptr);

		int Align = 0x10 - (uintptr_t(Address) % 0x10);

		for (int i = 0; i < 0xFFFF; i++)
		{
			if (IsFunctionRet(Address + i))
			{
				return MemAddress(Address + i);
			}
			if ((uintptr_t(Address + i) % 0x10 == 0) && (Address[i] == 0x40 && (Address[i + 1] >= 0x50 && Address[i + 1] <= 0x57) && (Address[i + 2] >= 0x50 && Address[i + 2] <= 0x57)))
			{
				return MemAddress(Address + i);
			}
		}

		return  MemAddress(nullptr);
	}

	inline void* RelativePattern(const char* Pattern, int32_t Range, int32_t Relative = 0)
	{
		if (!Address)
			return nullptr;

		return FindPatternInRange(Pattern, Address, Range, Relative != 0, Relative);
	}

	// every occurence of E8 counts as a call, use for max 1-5 calls only
	/* Negative index for search up, positive for search down  | goes beyond function-boundaries */
	inline void* GetCalledFunction(int32_t FunctionIndex)
	{
		if (!Address || FunctionIndex == 0)
			return nullptr;

		int32_t NumCalls = 0;

		for (int i = 0; i < (FunctionIndex > 0 ? 0xFFF : -0xFFF); (FunctionIndex > 0 ? i += 0x50 : i -= 0x50))
		{
			for (int j = 0; j < 0x50; j++)
			{
				//opcode: call
				if (Address[i + j] == 0xE8)
				{
					uint8_t* CallAddr = Address + i + j;

					void* Func = CallAddr + *reinterpret_cast<int32_t*>(CallAddr + 1) + 5; /*Addr + Offset + 5*/

					if ((uint64_t(Func) % 0x4) != 0x0)
						continue;
						
					if (++NumCalls == FunctionIndex)
					{
						return Func;
					}
				}
			}
		}

		return nullptr;
	}

	inline MemAddress FindNextFunctionStart()
	{
		if (!Address)
			return MemAddress(nullptr);

		uintptr_t FuncEnd = (uintptr_t)FindFunctionEnd();

		return FuncEnd % 0x10 != 0 ? FuncEnd + (0x10 - (FuncEnd % 0x10)) : FuncEnd;
	}
};

template<typename Type = const char*>
inline MemAddress FindByString(Type RefStr)
{
	uintptr_t ImageBase = GetImageBase();
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);
	PIMAGE_SECTION_HEADER Sections = IMAGE_FIRST_SECTION(NtHeader);

	uint8_t* DataSection = nullptr;
	uint8_t* TextSection = nullptr;
	DWORD DataSize = 0;
	DWORD TextSize = 0;

	uint8_t* StringAddress = nullptr;

	for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER& CurrentSection = Sections[i];

		std::string SectionName = (const char*)CurrentSection.Name;

		if (SectionName == ".rdata" && !DataSection)
		{
			DataSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			DataSize = CurrentSection.Misc.VirtualSize;
		}
		else if (SectionName == ".text" && !TextSection)
		{
			TextSection = (uint8_t*)(CurrentSection.VirtualAddress + ImageBase);
			TextSize = CurrentSection.Misc.VirtualSize;
		}
	}

	for (int i = 0; i < DataSize; i++)
	{
		if constexpr (std::is_same<Type, const char*>())
		{
			if (strcmp((const char*)RefStr, (const char*)(DataSection + i)) == 0)
			{
				//std::cout << "FoundStr ref: " << (const char*)(DataSection + i) << "\n";

				StringAddress = DataSection + i;
			}
		}
		else
		{
			if (wcscmp((const wchar_t*)RefStr, (const wchar_t*)(DataSection + i)) == 0)
			{
				//std::wcout << L"FoundStr wref: " << (const wchar_t*)(DataSection + i) << L"\n";

				StringAddress = DataSection + i;
			}
		}
	}

	for (int i = 0; i < TextSize; i++)
	{
		// opcode: lea
		if ((TextSection[i] == uint8_t(0x4C) || TextSection[i] == uint8_t(0x48)) && TextSection[i + 1] == uint8_t(0x8D))
		{
			const uint8_t* StrPtr = *(int32_t*)(TextSection + i + 3) + 7 + TextSection + i;

			if (StrPtr == StringAddress)
			{
				//std::cout << "Found Address: 0x" << (void*)(TextSection + i) << "\n";

				return { TextSection + i };
			}
		}
	}

	return nullptr;
}

inline MemAddress FindByWString(const wchar_t* RefStr)
{
	return FindByString<const wchar_t*>(RefStr);
}

template<typename CharType>
inline int32_t StrlenHelper(const CharType* Str)
{
	if constexpr (std::is_same<CharType, char>())
	{
		return strlen(Str);
	}
	else
	{
		return wcslen(Str);
	}
}

/* Slower than FindByString */
template<typename Type = const char*>
inline MemAddress FindByStringInAllSections(Type RefStr, uintptr_t StartAddress = 0x0, int32_t Range = 0x0)
{
	/* Stop scanning when arriving 0x10 bytes before the end of the memory range */
	constexpr int32_t OffsetFromMemoryEnd = 0x10;

	uintptr_t ImageBase = GetImageBase();
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);

	const DWORD SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

	const uintptr_t ImageEnd = ImageBase + SizeOfImage;

	/* If the StartAddress is not default nullptr, and is out of memory-range */
	if (StartAddress != 0x0 && (StartAddress < ImageBase || StartAddress > ImageEnd))
		return nullptr;

	/* Add a few bytes to the StartAddress to prevent instantly returning the previous result */
	uint8_t* SearchStart = StartAddress ? (reinterpret_cast<uint8_t*>(StartAddress) + 0x5) : reinterpret_cast<uint8_t*>(ImageBase);
	DWORD SearchRange = StartAddress ? ImageEnd - StartAddress : SizeOfImage;

	if (Range != 0x0)
		SearchRange = min(Range, SearchRange);

	if ((StartAddress + SearchRange) >= ImageEnd)
		SearchRange -= OffsetFromMemoryEnd;

	const int32_t RefStrLen = StrlenHelper(RefStr);

	for (uintptr_t i = 0; i < SearchRange; i++)
	{
		// opcode: lea
		if ((SearchStart[i] == uint8_t(0x4C) || SearchStart[i] == uint8_t(0x48)) && SearchStart[i + 1] == uint8_t(0x8D))
		{
			const uint8_t* StrPtr = *reinterpret_cast<int32_t*>(SearchStart + i + 3) + 7 + SearchStart + i;

			if (!IsInProcessRange(reinterpret_cast<uintptr_t>(StrPtr)))
				continue;

			if constexpr (std::is_same<Type, const char*>())
			{
				if (strncmp(reinterpret_cast<const char*>(RefStr), reinterpret_cast<const char*>(StrPtr), RefStrLen) == 0)
				{
					// std::cout << "FoundStr ref: " << (const char*)(SearchStart + i) << "\n";

					return { SearchStart + i };
				}
			}
			else
			{
				if (wcsncmp(reinterpret_cast<const wchar_t*>(RefStr), reinterpret_cast<const wchar_t*>(StrPtr), RefStrLen) == 0)
				{
					// std::wcout << L"FoundStr wref: " << (const wchar_t*)(SearchStart + i) << L"\n";

					return { SearchStart + i };
				}
			}
		}
	}

	return nullptr;
}

template<typename Type = const char*>
inline MemAddress FindUnrealExecFunctionByString(Type RefStr, void* StartAddress = nullptr)
{
	uintptr_t ImageBase = GetImageBase();
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)(ImageBase);
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)(ImageBase + DosHeader->e_lfanew);

	const DWORD SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

	uint8_t* SearchStart = StartAddress ? reinterpret_cast<uint8_t*>(StartAddress) : reinterpret_cast<uint8_t*>(ImageBase);
	DWORD SearchRange = SizeOfImage;

	const int32_t RefStrLen = StrlenHelper(RefStr);

	static auto IsValidExecFunctionNotSetupFunc = [](uintptr_t Address) -> bool
	{
		/* 
		* UFuntion construction functions setting up exec functions always start with these asm instructions:
		* sub rsp, 28h
		* 
		* In opcode bytes: 48 83 EC 28
		*/
		if (*reinterpret_cast<int32_t*>(Address) == 0x284883EC || *reinterpret_cast<int32_t*>(Address) == 0x4883EC28)
			return false;

		MemAddress AsAddress(Address);

		/* A signature specifically made for UFunctions-construction functions. If this signature is found we're in a function that we *don't* want. */
		if (AsAddress.RelativePattern("48 8B 05 ? ? ? ? 48 85 C0 75 ? 48 8D 15", 0x28) != nullptr)
			return false;

		return true;
	};

	for (uintptr_t i = 0; i < (SearchRange - 0x8); i += sizeof(void*))
	{
		const uintptr_t PossibleStringAddress = *reinterpret_cast<uintptr_t*>(SearchStart + i);
		const uintptr_t PossibleExecFuncAddress = *reinterpret_cast<uintptr_t*>(SearchStart + i + sizeof(void*));

		if (PossibleStringAddress == PossibleExecFuncAddress)
			continue;

		if (!IsInProcessRange(PossibleStringAddress) || !IsInProcessRange(PossibleExecFuncAddress))
			continue;

		if constexpr (std::is_same<Type, const char*>())
		{
			if (strncmp(reinterpret_cast<const char*>(RefStr), reinterpret_cast<const char*>(PossibleStringAddress), RefStrLen) == 0 && IsValidExecFunctionNotSetupFunc(PossibleExecFuncAddress))
			{
				// std::cout << "FoundStr ref: " << reinterpret_cast<const char*>(PossibleStringAddress) << "\n";

				return { PossibleExecFuncAddress };
			}
		}
		else
		{
			if (wcsncmp(reinterpret_cast<const wchar_t*>(RefStr), reinterpret_cast<const wchar_t*>(PossibleStringAddress), RefStrLen) == 0 && IsValidExecFunctionNotSetupFunc(PossibleExecFuncAddress))
			{
				// std::wcout << L"FoundStr wref: " << reinterpret_cast<const wchar_t*>(PossibleStringAddress) << L"\n";

				return { PossibleExecFuncAddress };
			}
		}
	}

	return nullptr;
}

/* Slower than FindByWString */
inline MemAddress FindByWStringInAllSections(const wchar_t* RefStr)
{
	return FindByStringInAllSections<const wchar_t*>(RefStr);
}


namespace FileNameHelper
{
	inline void MakeValidFileName(std::string& InOutName)
	{
		for (char& c : InOutName)
		{
			if (c == '<' || c == '>' || c == ':' || c == '\"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
				c = '_';
		}
	}
}
