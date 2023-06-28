/*************************************************************************
 *                                                                       *
 * ODER's Utilities Library. Copyright (C) 2008 Oleh Derevenko.          *
 * All rights reserved.  e-mail: odar@eleks.com (change all "a" to "e")  *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 3 of the License, or (at    *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE-LESSER.TXT. Since LGPL is the extension of GPL     *
 *       the text of GNU General Public License is also provided for     *
 *       your information in file LICENSE.TXT.                           *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *   (3) The zlib/libpng license that is included with this library in   *
 *       the file LICENSE-ZLIB.TXT                                       *
 *                                                                       *
 * This library is distributed WITHOUT ANY WARRANTY, including implied   *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.      *
 * See the files LICENSE.TXT and LICENSE-LESSER.TXT or LICENSE-BSD.TXT   *
 * or LICENSE-ZLIB.TXT for more details.                                 *
 *                                                                       *
 *************************************************************************/

#include <ou/features.h>


#if _OU_FEATURE_SET >= _OU_FEATURE_SET_TLS

#include <ou/threadlocalstorage.h>
#include <ou/atomicflags.h>
#include <ou/atomic.h>
#include <ou/simpleflags.h>
#include <ou/malloc.h>
#include <ou/templates.h>
#include <ou/inttypes.h>

#include <string.h>
#include <errno.h>
#include <new>

#if !defined(EOK)

#define EOK		0


#endif


#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

#include <windows.h>


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS


BEGIN_NAMESPACE_OU();


class CTLSStorageInstance;

enum ESTORAGEINSTANCEKIND
{
	SIK__MIN,

	SIK_AUTOCLEANUP = SIK__MIN,
	SIK_MANUALCLEANUP,

	SIK__MAX,
};


static unsigned int g_uiThreadLocalStorageInitializationCount = 0;
static CTLSStorageInstance *g_apsiStorageGlobalInstances[SIK__MAX] = { NULL };
static HTLSKEYVALUE g_ahkvStorageGlobalKeyValues[SIK__MAX] = { NULL };


static inline size_t DecodeInstanceKindFromKeySelector(const HTLSKEYSELECTOR &hksKeySelector)
{
	return (HTLSKEYSELECTOR::value_type)hksKeySelector - g_ahkvStorageGlobalKeyValues;
}

static inline HTLSKEYSELECTOR EncodeKeySelectorFromStorageKind(ESTORAGEINSTANCEKIND ikInstanceKind)
{
	return g_ahkvStorageGlobalKeyValues + ikInstanceKind;
}


#if !defined(_OU_TLS_ARRAY_ELEMENT_COUNT)

// Default TLS array element count
#define _OU_TLS_ARRAY_ELEMENT_COUNT		8


#endif // #if !defined(_OU_TLS_ARRAY_ELEMENT_COUNT)


// Few bits must be reserved for additional purposes (currently 1)
#if (_OU_TLS_ARRAY_ELEMENT_COUNT < 1) || (_OU_TLS_ARRAY_ELEMENT_COUNT > 30)

#error Please specify TLS array element count in range from 1 to 30


#endif // #if (_OU_TLS_ARRAY_ELEMENT_COUNT < 1) || (_OU_TLS_ARRAY_ELEMENT_COUNT > 30)


enum
{
	TLS_ARRAY_ELEMENT__MAX		= _OU_TLS_ARRAY_ELEMENT_COUNT, // 16 threads with 8 values each using 4 + 4 bytes is ~1 kb of memory
};


struct CTLSStorageArray
{
private:
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	typedef HANDLE CClientHandleArray[TLS_ARRAY_ELEMENT__MAX];
	typedef unsigned int CHandleTranslationMap[TLS_ARRAY_ELEMENT__MAX];
	
	
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
public:
	static inline size_t GetHeaderSize() { return OU_ALIGNED_SIZE(sizeof(CTLSStorageArray), CTLSStorageBlock::TSB_LARGEST_ALIGNMENT); }

public:
	static CTLSStorageArray *AllocateInstance(tlsindextype iValueCount);
	void FreeInstance(tlsindextype iValueCount);

protected:
	inline CTLSStorageArray(); // Use AllocateInstance()
	inline ~CTLSStorageArray(); // Use FreeInstance()

public:
	void FreeStorageBlockOnThreadExit(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount);
	
public:
	bool FindFreeStorageBlock(CTLSStorageBlock *&psbOutFreeStorageBlock, 
		tlsindextype iValueCount, bool bIsManualCleanup);
	
private:
	bool FindFreeStorageBlockIndex(unsigned int &nOutFreeBlockIndex, tlsindextype iValueCount, bool bIsManualCleanup);
	bool FindFreeStorageBlockIndexWithPossibilityVerified(unsigned int &nOutFreeBlockIndex, bool bIsManualCleanup);
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	bool FindAbandonedStorageBlockIndex(unsigned int &nOutFreeBlockIndex, tlsindextype iValueCount);
	unsigned int TranslateClientHandles(CClientHandleArray haTranslatedHandlesStorage, CHandleTranslationMap tmTranslationMapStorage,
		const HANDLE *&ph_OutTranslatedHandles, const unsigned int *&puiOutTranslationMap) const;

#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

private:
	void FreeStorageAllBlocks(tlsindextype iValueCount);
	void ReinitializeStorageSingleBlock(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount);
	static void FinalizeStorageSingleBlock(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount);

	void AssignAllBlocksHostArray(tlsindextype iValueCount);
	inline void AssignSingleBlockHostArray(CTLSStorageBlock *psbStorageBlock);
	
private:
	inline CTLSStorageBlock *GetStorageBlockPointer(unsigned int nBlockIndex, tlsindextype iValueCount) const;
	inline unsigned int GetStorageBlockIndex(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount) const;
	inline static void ZeroStorageBlockMemory(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount);

private:
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	void AllocateBlockThreadHandle(unsigned int nBlockIndex);
	void FreeStorageThreadHandle(unsigned int nBlockIndex);
	
	void AssignAllBlocksInvalidThreads();
	bool CheckIfAllBlocksHaveInvalidThreads();
	
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
private:
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	inline void SetBlockThreadHandle(unsigned int nBlockIndex, HANDLE hValue)
	{
		m_haBlockThreads[nBlockIndex] = hValue;
	}

	inline HANDLE GetBlockThreadHandle(unsigned int nBlockIndex) const
	{
		return m_haBlockThreads[nBlockIndex];
	}
	
	inline const HANDLE *GetBlockThreadHandlesStorage() const
	{
		return m_haBlockThreads;
	}
	
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

public:
	inline void SetNextArray(CTLSStorageArray *psaInstance)
	{
		m_psaNextArray = (atomicptr)psaInstance;
	}
	
	inline CTLSStorageArray *GetNextArray() const
	{
		return (CTLSStorageArray *)m_psaNextArray;
	}
	
private:
	enum
	{
		FL_OCCUPANCY_FLAGS__START	= 0x00000001,
		FL_OCCUPANCY_FLAGS__END		= FL_OCCUPANCY_FLAGS__START << TLS_ARRAY_ELEMENT__MAX,

		FL_ARRAY_LOCKED				= FL_OCCUPANCY_FLAGS__END,
	};

	inline bool GetAreAllBlocksOccupied() const
	{
		return m_afOccupancyFlags.EnumAllQueryEnumeratedFlags(FL_OCCUPANCY_FLAGS__START, TLS_ARRAY_ELEMENT__MAX) == OU_FLAGS_ENUMFLAGS_MASK(COccupancyFlagsType::value_type, FL_OCCUPANCY_FLAGS__START, TLS_ARRAY_ELEMENT__MAX);
	}

	inline bool GetIsAnyBlockOccupied() const
	{
		return m_afOccupancyFlags.EnumAnyGetEnumeratedFlagValue(FL_OCCUPANCY_FLAGS__START, TLS_ARRAY_ELEMENT__MAX);
	}

	inline bool SetBlockOccupiedFlag(unsigned int nBlockIndex)
	{
		return m_afOccupancyFlags.EnumModifyEnumeratedFlagValue(FL_OCCUPANCY_FLAGS__START, nBlockIndex, TLS_ARRAY_ELEMENT__MAX, true);
	}

	inline void ResetBlockOccupiedFlag(unsigned int nBlockIndex)
	{
		m_afOccupancyFlags.EnumDropEnumeratedFlagValue(FL_OCCUPANCY_FLAGS__START, nBlockIndex, TLS_ARRAY_ELEMENT__MAX);
	}

	inline bool GetBlockOccupiedFlag(unsigned int nBlockIndex) const
	{
		return m_afOccupancyFlags.EnumGetEnumeratedFlagValue(FL_OCCUPANCY_FLAGS__START, nBlockIndex, TLS_ARRAY_ELEMENT__MAX);
	}

	inline bool SetArrayLockedFlag()
	{
		return m_afOccupancyFlags.ModifySingleFlagValue(FL_ARRAY_LOCKED, true);
	}
	
	inline void ResetArrayLockedFlag()
	{
		m_afOccupancyFlags.DropFlagsMaskValue(FL_ARRAY_LOCKED);
	}

private:
	typedef CAtomicFlags COccupancyFlagsType;

	volatile atomicptr	m_psaNextArray; // CTLSStorageArray *
	COccupancyFlagsType	m_afOccupancyFlags;

#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	CClientHandleArray	m_haBlockThreads;


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	// CTLSStorageBlock m_asbStorageBlocks[];
};

class CTLSStorageInstance
{
public:
	static CTLSStorageInstance *AllocateInstance(tlsindextype iValueCount, unsigned int uiInitializationFlags);
	void FreeInstance();

protected:
	CTLSStorageInstance(tlsindextype iValueCount, unsigned int uiInitializationFlags);
	~CTLSStorageInstance();

public:
	bool Init(ESTORAGEINSTANCEKIND ikInstanceKind);

private:
	void Finit();

public:
	inline const HTLSKEYVALUE &RetrieveStorageKey() const { return GetStorageKey(); }
	inline tlsindextype RetrieveValueCount() const { return GetValueCount(); }
	inline unsigned int RetrieveInitializationFlags() const { return GetInitializationFlags(); }
	
	inline bool GetIsThreadManualCleanup() const { return GetThreadManualCleanupFlag(); }

public:
	void FreeStorageBlockOnThreadExit(CTLSStorageBlock *psbStorageBlock);

public:
	bool FindFreeStorageBlock(CTLSStorageBlock *&psbOutStorageBlock);

private:
	bool FindFreeStorageBlockInArrayList(CTLSStorageBlock *&psbOutStorageBlock);
	bool FindFreeStorageBlockInArrayListSegment(CTLSStorageBlock *&psbOutStorageBlock, 
		CTLSStorageArray *psaListSegmentBegin, CTLSStorageArray *psaListSegmentEnd);
	bool FindFreeStorageBlockFromArray(CTLSStorageBlock *&psbOutStorageBlock, 
		CTLSStorageArray *psaArrayInstance);

	void AddStorageArrayToArrayList(CTLSStorageArray *psaStorageArray);

private:
	static bool AllocateStorageKey(HTLSKEYVALUE &hkvOutStorageKey, ESTORAGEINSTANCEKIND ikInstanceKind);
	static void FreeStorageKey(const HTLSKEYVALUE &hkvStorageKey);

#if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS

	static void FreeStorageBlock_Callback_Automatic(void *pv_DataValue);
	static void FreeStorageBlock_Callback_Manual(void *pv_DataValue);


#endif // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS

	void FreeStorageBlock(CTLSStorageBlock *psbStorageBlock);
	
	CTLSStorageArray *AllocateStorageArray();
	void FreeStorageArrayList(CTLSStorageArray *psaStorageArrayList);
	
private:
	inline bool TrySettingStorageArrayList(CTLSStorageArray *psaInstance, CTLSStorageArray *psaCurrentList)
	{
		return AtomicCompareExchangePointer(&m_psaStorageList, (atomicptr)psaCurrentList, (atomicptr)psaInstance);
	}

	inline CTLSStorageArray *GetStorageArrayList() const
	{
		return (CTLSStorageArray *)m_psaStorageList;
	}

	inline void SetStorageKey(const HTLSKEYVALUE &hskValue) { m_hskStorageKey = hskValue; }
	inline const HTLSKEYVALUE &GetStorageKey() const { return m_hskStorageKey; }

	inline tlsindextype GetValueCount() const { return m_iValueCount; }

private:
	enum
	{
		FL_STORAGE_KEY_VALID			= 0x00000001,

		FLM_INITIALIZATION_FLAGS_MASK	= 0x0000FFFF,
		FLS_INITIALIZATION_FLAGS_SHIFT	= 16,

		FL_INITIALIZATION_THREAD_MANUAL_CLEANUP = CTLSInitialization::SIF_MANUAL_CLEANUP_ON_THREAD_EXIT << FLS_INITIALIZATION_FLAGS_SHIFT,
	};

	inline void SetStorageKeyValidFlag() { m_sfInstanceFlags.SignalFlagsMaskValue(FL_STORAGE_KEY_VALID); }
	inline void ResetStorageKeyValidFlag() { m_sfInstanceFlags.DropFlagsMaskValue(FL_STORAGE_KEY_VALID); }
	inline bool GetStorageKeyValidFlag() const { return m_sfInstanceFlags.GetFlagsMaskValue(FL_STORAGE_KEY_VALID); }

	inline void SetInitializationFlags(unsigned int uiValue) { m_sfInstanceFlags.StoreFlagsEnumeratedValue(FLM_INITIALIZATION_FLAGS_MASK, FLS_INITIALIZATION_FLAGS_SHIFT, uiValue); }
	inline unsigned int GetInitializationFlags() const { return m_sfInstanceFlags.RetrieveFlagsEnumeratedValue(FLM_INITIALIZATION_FLAGS_MASK, FLS_INITIALIZATION_FLAGS_SHIFT); }

	inline bool GetThreadManualCleanupFlag() const { return m_sfInstanceFlags.GetFlagsMaskValue(FL_INITIALIZATION_THREAD_MANUAL_CLEANUP); }

private:
	volatile atomicptr	m_psaStorageList; // CTLSStorageArray *
	HTLSKEYVALUE		m_hskStorageKey;
	CSimpleFlags		m_sfInstanceFlags;
	tlsindextype		m_iValueCount;
};


//////////////////////////////////////////////////////////////////////////
// CTLSStorageArray methods

CTLSStorageArray *CTLSStorageArray::AllocateInstance(tlsindextype iValueCount)
{
	const size_t nHeaderSize = CTLSStorageArray::GetHeaderSize();
	const size_t nBlockSize = CTLSStorageBlock::GetRequiredSize(iValueCount);
	size_t nRequiredSize = nHeaderSize + nBlockSize * TLS_ARRAY_ELEMENT__MAX;

	CTLSStorageArray *psaNewInstance = (CTLSStorageArray *)AllocateMemoryBlock(nRequiredSize);
	
	if (psaNewInstance)
	{
		memset(psaNewInstance, 0, nRequiredSize);
		new((CTLSStorageArray *)psaNewInstance) CTLSStorageArray();

		psaNewInstance->AssignAllBlocksHostArray(iValueCount);
	}

	return psaNewInstance;
}

void CTLSStorageArray::FreeInstance(tlsindextype iValueCount)
{
	if (GetIsAnyBlockOccupied())
	{
		FreeStorageAllBlocks(iValueCount);
	}

	this->CTLSStorageArray::~CTLSStorageArray();
	FreeMemoryBlock((void *)this);
}

CTLSStorageArray::CTLSStorageArray()
{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	AssignAllBlocksInvalidThreads();
	
	
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
}

CTLSStorageArray::~CTLSStorageArray()
{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	OU_ASSERT(CheckIfAllBlocksHaveInvalidThreads());
	
	
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
}


void CTLSStorageArray::FreeStorageBlockOnThreadExit(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount)
{
	ReinitializeStorageSingleBlock(psbStorageBlock, iValueCount);
	// OU_ASSERT(GetBlockThreadHandle(nBlockIndex) == INVALID_HANDLE_VALUE) -- assertion further in the code

	unsigned int nBlockIndex = GetStorageBlockIndex(psbStorageBlock, iValueCount);
	OU_ASSERT(GetBlockOccupiedFlag(nBlockIndex));
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	OU_ASSERT(GetBlockThreadHandle(nBlockIndex) == INVALID_HANDLE_VALUE); // The method is not to be called if automatic cleanup is enabled


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	ResetBlockOccupiedFlag(nBlockIndex);
}


bool CTLSStorageArray::FindFreeStorageBlock(CTLSStorageBlock *&psbOutFreeStorageBlock, 
	tlsindextype iValueCount, bool bIsManualCleanup)
{
	bool bResult = false;

	unsigned int nFreeBlockIndex;

	if (FindFreeStorageBlockIndex(nFreeBlockIndex, iValueCount, bIsManualCleanup))
	{
		CTLSStorageBlock *psbFreeStorageBlock = GetStorageBlockPointer(nFreeBlockIndex, iValueCount);
			
		psbOutFreeStorageBlock = psbFreeStorageBlock;
		bResult = true;
	}

	return bResult;
}


bool CTLSStorageArray::FindFreeStorageBlockIndex(unsigned int &nOutFreeBlockIndex, 
	tlsindextype iValueCount, bool bIsManualCleanup)
{
	bool bResult = false;

	if (!GetAreAllBlocksOccupied() && FindFreeStorageBlockIndexWithPossibilityVerified(nOutFreeBlockIndex, bIsManualCleanup))
	{
		bResult = true;
	}
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	else if (!bIsManualCleanup)
	{
		// Execution gets here is all slots were already occupied or
		// they become occupied during search (otherwise why 
		// FindFreeStorageBlockIndexWithPossibilityVerified call failed???).
		// In Automatic cleanup mode a block can't become free by itself -
		// it is just re-allocated for new thread and remains busy.
		OU_ASSERT(GetAreAllBlocksOccupied());
		
		// The locking is performed to avoid more than one threads checking
		// for abandoned handles simultaneously.
		// If locking fails, execution just proceeds to next array in the chain
		if (SetArrayLockedFlag())
		{
			bResult = FindAbandonedStorageBlockIndex(nOutFreeBlockIndex, iValueCount);
				
			ResetArrayLockedFlag();
		}
	}


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

	return bResult;
}

bool CTLSStorageArray::FindFreeStorageBlockIndexWithPossibilityVerified(unsigned int &nOutFreeBlockIndex, 
	bool bIsManualCleanup)
{
	unsigned int nBlockIndex = 0;

	for (; nBlockIndex != TLS_ARRAY_ELEMENT__MAX; ++nBlockIndex)
	{
		if (SetBlockOccupiedFlag(nBlockIndex))
		{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

			if (!bIsManualCleanup)
			{
				AllocateBlockThreadHandle(nBlockIndex);
			}
			

#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

			nOutFreeBlockIndex = nBlockIndex;
			break;
		}
	}

	bool bResult = nBlockIndex != TLS_ARRAY_ELEMENT__MAX;
	return bResult;
}


#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

bool CTLSStorageArray::FindAbandonedStorageBlockIndex(unsigned int &nOutFreeBlockIndex, 
	tlsindextype iValueCount)
{
	bool bResult = false;

	do 
	{
		CClientHandleArray haTranslatedHandlesStorage;
		CHandleTranslationMap tmTranslationMapStorage;
		
		const HANDLE *ph_TranslatedHandles;
		const unsigned int *puiTranslationMap;
		
		// Translate handles into array for the case if there are invalids
		unsigned int nHandleCount = TranslateClientHandles(haTranslatedHandlesStorage, tmTranslationMapStorage,
			ph_TranslatedHandles, puiTranslationMap);
		OU_ASSERT(OU_IN_INT_RANGE(nHandleCount, 0, MAXIMUM_WAIT_OBJECTS + 1));
		
		if (nHandleCount == 0)
		{
			break;
		}

		// Since allocating a new storage block is a relatively slow operation
		// it is acceptable to enter kernel for checking for exited threads.
		DWORD dwWaitResult = ::WaitForMultipleObjects(nHandleCount, ph_TranslatedHandles, FALSE, 0);
		
		if (!OU_IN_INT_RANGE(dwWaitResult - WAIT_OBJECT_0, 0, nHandleCount))
		{
			// Wait should not normally fail. If it does it's in most cases an indication
			// of invalid handle passed as parameter. However it may fail because of other
			// reasons as well. If this assertion fails too often and you are sure all the 
			// handles are valid, it is safe to comment it.
			OU_ASSERT(dwWaitResult != WAIT_FAILED);

			break;
		}

		unsigned int nTranslatedBlockIndex = (unsigned int)(dwWaitResult - WAIT_OBJECT_0);
		unsigned int nBlockIndex = !puiTranslationMap ? nTranslatedBlockIndex : puiTranslationMap[nTranslatedBlockIndex];
		
		CTLSStorageBlock *psbStorageBlock = GetStorageBlockPointer(nBlockIndex, iValueCount);
		ReinitializeStorageSingleBlock(psbStorageBlock, iValueCount);

		// Close old handle and make a duplicate of current thread handle
		FreeStorageThreadHandle(nBlockIndex);
		AllocateBlockThreadHandle(nBlockIndex);

		nOutFreeBlockIndex = nBlockIndex;
		bResult = true;
	}
	while (false);

	return bResult;
}

unsigned int CTLSStorageArray::TranslateClientHandles(CClientHandleArray haTranslatedHandlesStorage, CHandleTranslationMap tmTranslationMapStorage,
	const HANDLE *&ph_OutTranslatedHandles, const unsigned int *&puiOutTranslationMap) const
{
	ph_OutTranslatedHandles = haTranslatedHandlesStorage;
	puiOutTranslationMap = tmTranslationMapStorage;

	unsigned int nTargetStartIndex = 0;
	unsigned int nSourceStartIndex = 0, nSourceCurrentIndex = 0;

	while (true)
	{
		if (GetBlockThreadHandle(nSourceCurrentIndex) == INVALID_HANDLE_VALUE)
		{
			const HANDLE *ph_BlockThreadHandles = GetBlockThreadHandlesStorage();

			unsigned int nTargetIncrement = nSourceCurrentIndex - nSourceStartIndex;

			memcpy(&haTranslatedHandlesStorage[nTargetStartIndex], &ph_BlockThreadHandles[nSourceStartIndex], nTargetIncrement * sizeof(HANDLE));
			for (; nTargetIncrement != 0; ++nTargetStartIndex, ++nSourceStartIndex, --nTargetIncrement) { tmTranslationMapStorage[nTargetStartIndex] = nSourceStartIndex; }

			// Skip invalid handle (at this point nSourceStartIndex is equal to nSourceCurrentIndex)
			++nSourceStartIndex;
		}

		++nSourceCurrentIndex;
		
		if (nSourceCurrentIndex == TLS_ARRAY_ELEMENT__MAX)
		{
			// Start indice can be equal if and only if no invalid handles have been found
			if (nSourceStartIndex != nTargetStartIndex)
			{
				const HANDLE *ph_BlockThreadHandles = GetBlockThreadHandlesStorage();
				
				unsigned int nTargetIncrement = nSourceCurrentIndex - nSourceStartIndex;
				
				memcpy(&haTranslatedHandlesStorage[nTargetStartIndex], &ph_BlockThreadHandles[nSourceStartIndex], nTargetIncrement * sizeof(HANDLE));
				for (; nTargetIncrement != 0; ++nTargetStartIndex, ++nSourceStartIndex, --nTargetIncrement) { tmTranslationMapStorage[nTargetStartIndex] = nSourceStartIndex; }
			}

			break;
		}
	}

	// If all the handles are valid...
	if (nTargetStartIndex == 0)
	{
		// ...just return original handle array as no copying was performed
		ph_OutTranslatedHandles = GetBlockThreadHandlesStorage();
		puiOutTranslationMap = NULL;
	}

	return nTargetStartIndex;
}


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS


void CTLSStorageArray::FreeStorageAllBlocks(tlsindextype iValueCount)
{
	for (unsigned int nBlockIndex = 0; nBlockIndex != TLS_ARRAY_ELEMENT__MAX; ++nBlockIndex)
	{
		if (GetBlockOccupiedFlag(nBlockIndex))
		{
			CTLSStorageBlock *psbStorageBlock = GetStorageBlockPointer(nBlockIndex, iValueCount);

			FinalizeStorageSingleBlock(psbStorageBlock, iValueCount);
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
			
			FreeStorageThreadHandle(nBlockIndex);


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
		}
		else
		{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
			
			OU_ASSERT(GetBlockThreadHandle(nBlockIndex) == INVALID_HANDLE_VALUE); // Where did the handle come from if block is not occupied?
			
			
#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
		}
	}
}

void CTLSStorageArray::ReinitializeStorageSingleBlock(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount)
{
	FinalizeStorageSingleBlock(psbStorageBlock, iValueCount);
	
	ZeroStorageBlockMemory(psbStorageBlock, iValueCount);
	AssignSingleBlockHostArray(psbStorageBlock);
}

void CTLSStorageArray::FinalizeStorageSingleBlock(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount)
{
	for (tlsindextype iValueIndex = 0; iValueIndex != iValueCount; ++iValueIndex)
	{
		tlsvaluetype vValueData = psbStorageBlock->GetValueData(iValueIndex);

		if (vValueData)
		{
			CTLSValueDestructor fnValueDestructor = psbStorageBlock->GetValueDestructor(iValueIndex);

			if (fnValueDestructor)
			{
				fnValueDestructor(vValueData);
			}
		}
	}
}


void CTLSStorageArray::AssignAllBlocksHostArray(tlsindextype iValueCount)
{
	for (unsigned int nBlockIndex = 0; nBlockIndex != TLS_ARRAY_ELEMENT__MAX; ++nBlockIndex)
	{
		CTLSStorageBlock *psbStorageBlock = GetStorageBlockPointer(nBlockIndex, iValueCount);

		AssignSingleBlockHostArray(psbStorageBlock);
	}
}

void CTLSStorageArray::AssignSingleBlockHostArray(CTLSStorageBlock *psbStorageBlock)
{
	psbStorageBlock->SetHostArray(this);
}


CTLSStorageBlock *CTLSStorageArray::GetStorageBlockPointer(unsigned int nBlockIndex, tlsindextype iValueCount) const
{
	OU_ASSERT(OU_IN_INT_RANGE(nBlockIndex, 0, TLS_ARRAY_ELEMENT__MAX));

	const size_t nHeaderSize = CTLSStorageArray::GetHeaderSize();
	const size_t nBlockSize = CTLSStorageBlock::GetRequiredSize(iValueCount);
	const size_t nBlockZeroOffset = CTLSStorageBlock::GetZeroOffset(iValueCount);
	
	CTLSStorageBlock *psbStorageBlock = (CTLSStorageBlock *)(((int8ou *)this) + nHeaderSize + nBlockIndex * nBlockSize + nBlockZeroOffset);
	return psbStorageBlock;
}

unsigned int CTLSStorageArray::GetStorageBlockIndex(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount) const
{
	const size_t nHeaderSize = CTLSStorageArray::GetHeaderSize();
	const size_t nBlockSize = CTLSStorageBlock::GetRequiredSize(iValueCount);
	const size_t nBlockZeroOffset = CTLSStorageBlock::GetZeroOffset(iValueCount);

	unsigned int uiBlockIndex = (unsigned int)((((int8ou *)psbStorageBlock) - nBlockZeroOffset - nHeaderSize - ((int8ou *)this)) / nBlockSize);
	OU_ASSERT((((int8ou *)psbStorageBlock) - nBlockZeroOffset - nHeaderSize - ((int8ou *)this)) % nBlockSize == 0);
	OU_ASSERT(OU_IN_INT_RANGE(uiBlockIndex, 0, TLS_ARRAY_ELEMENT__MAX));

	return uiBlockIndex;
}

void CTLSStorageArray::ZeroStorageBlockMemory(CTLSStorageBlock *psbStorageBlock, tlsindextype iValueCount)
{
	const size_t nBlockSize = CTLSStorageBlock::GetRequiredSize(iValueCount);
	const size_t nBlockZeroOffset = CTLSStorageBlock::GetZeroOffset(iValueCount);

	memset(((int8ou *)psbStorageBlock) - nBlockZeroOffset, 0, nBlockSize);
}


#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

void CTLSStorageArray::AllocateBlockThreadHandle(unsigned int nBlockIndex)
{
	OU_ASSERT(GetBlockThreadHandle(nBlockIndex) == INVALID_HANDLE_VALUE);
	
	HANDLE hCurrentThreadDuplicate;
	
	HANDLE hCurrentProcess = ::GetCurrentProcess();
	HANDLE hCurrentThread = ::GetCurrentThread();
	if (!::DuplicateHandle(hCurrentProcess, hCurrentThread, hCurrentProcess, &hCurrentThreadDuplicate, SYNCHRONIZE, FALSE, 0))
	{
		// Handle duplication should not normally fail. 
		// Thread and process pseudo-handles have full access allowed.
		// The duplication may only fail in case of kernel internal problems
		// (like lack of the resources or resource limit hits).
		// Well, in this case thread data will remain in memory until 
		// CTLSInitialization::FinalizeTLSAPI() is called.
		hCurrentThreadDuplicate = INVALID_HANDLE_VALUE;
	}
	
	SetBlockThreadHandle(nBlockIndex, hCurrentThreadDuplicate);
}

void CTLSStorageArray::FreeStorageThreadHandle(unsigned int nBlockIndex)
{
	HANDLE hExistingThreadHandle = GetBlockThreadHandle(nBlockIndex);
	
	if (hExistingThreadHandle != INVALID_HANDLE_VALUE)
	{
		BOOL bHandleCloseResult = ::CloseHandle(hExistingThreadHandle);
		OU_VERIFY(bHandleCloseResult); // Closing handle should normally succeed
		
		SetBlockThreadHandle(nBlockIndex, INVALID_HANDLE_VALUE);
	}
}


void CTLSStorageArray::AssignAllBlocksInvalidThreads()
{
	for (unsigned int nBlockIndex = 0; nBlockIndex != TLS_ARRAY_ELEMENT__MAX; ++nBlockIndex)
	{
		SetBlockThreadHandle(nBlockIndex, INVALID_HANDLE_VALUE);
	}
}

bool CTLSStorageArray::CheckIfAllBlocksHaveInvalidThreads()
{
	unsigned nBlockIndex = 0;

	for (; nBlockIndex != TLS_ARRAY_ELEMENT__MAX; ++nBlockIndex)
	{
		if (GetBlockThreadHandle(nBlockIndex) != INVALID_HANDLE_VALUE)
		{
			break;
		}
	}

	bool bResult = nBlockIndex == TLS_ARRAY_ELEMENT__MAX;
	return bResult;
}


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS


//////////////////////////////////////////////////////////////////////////
// CTLSStorageInstance methods

CTLSStorageInstance *CTLSStorageInstance::AllocateInstance(tlsindextype iValueCount, unsigned int uiInitializationFlags)
{
	size_t nSizeRequired = sizeof(CTLSStorageInstance);

	CTLSStorageInstance *psiNewInstance = (CTLSStorageInstance *)AllocateMemoryBlock(nSizeRequired);
	
	if (psiNewInstance)
	{
		new(psiNewInstance) CTLSStorageInstance(iValueCount, uiInitializationFlags);
	}
	
	return psiNewInstance;
}

void CTLSStorageInstance::FreeInstance()
{
	this->CTLSStorageInstance::~CTLSStorageInstance();
	FreeMemoryBlock(this);
}


CTLSStorageInstance::CTLSStorageInstance(tlsindextype iValueCount, unsigned int uiInitializationFlags):
	m_psaStorageList((atomicptr)0),
	m_hskStorageKey((HTLSKEYVALUE::value_type)0),
	m_iValueCount(iValueCount)
{
	SetInitializationFlags(uiInitializationFlags);
}

CTLSStorageInstance::~CTLSStorageInstance()
{
	Finit();
}


bool CTLSStorageInstance::Init(ESTORAGEINSTANCEKIND ikInstanceKind)
{
	bool bResult = false;

	bool bKeyAllocationResult = false;
	HTLSKEYVALUE hkvStorageKey;
	
	do
	{
		if (!AllocateStorageKey(hkvStorageKey, ikInstanceKind))
		{
			break;
		}

		bKeyAllocationResult = true;

		CTLSStorageArray *psaFirstStorageArray = AllocateStorageArray();
		
		if (!psaFirstStorageArray)
		{
			break;
		}

		SetStorageKey(hkvStorageKey);
		SetStorageKeyValidFlag();
		AddStorageArrayToArrayList(psaFirstStorageArray);

		bResult = true;
	}
	while (false);

	if (!bResult)
	{
		if (bKeyAllocationResult)
		{
			FreeStorageKey(hkvStorageKey);
		}
	}
	
	return bResult;
}

void CTLSStorageInstance::Finit()
{
	CTLSStorageArray *psaStorageArrayList = GetStorageArrayList();

	if (psaStorageArrayList)
	{
		FreeStorageArrayList(psaStorageArrayList);

		bool bListClearingResult = TrySettingStorageArrayList(NULL, psaStorageArrayList); // It could be assigned directly, but I just do not want to add an extra method
		OU_VERIFY(bListClearingResult);
	}

	if (GetStorageKeyValidFlag())
	{
		const HTLSKEYVALUE &hkvStorageKey = GetStorageKey();
		FreeStorageKey(hkvStorageKey);

		ResetStorageKeyValidFlag();
	}
}


void CTLSStorageInstance::FreeStorageBlockOnThreadExit(CTLSStorageBlock *psbStorageBlock)
{
	FreeStorageBlock(psbStorageBlock);
}


bool CTLSStorageInstance::FindFreeStorageBlock(CTLSStorageBlock *&psbOutStorageBlock)
{
	bool bResult = false;
	
	do
	{
		if (!FindFreeStorageBlockInArrayList(psbOutStorageBlock))
		{
			CTLSStorageArray *psaStorageArray = AllocateStorageArray();
			
			if (!psaStorageArray)
			{
				break;
			}

			FindFreeStorageBlockFromArray(psbOutStorageBlock, psaStorageArray); // Must always succeed as array is not added to list yet

			AddStorageArrayToArrayList(psaStorageArray);
		}
	
		bResult = true;
	}
	while (false);
	
	return bResult;
}

bool CTLSStorageInstance::FindFreeStorageBlockInArrayList(CTLSStorageBlock *&psbOutStorageBlock)
{
	bool bResult;

	CTLSStorageArray *psaListOldHead = NULL;
	CTLSStorageArray *psaListCurrentHead = GetStorageArrayList();

	while (true)
	{
		if (FindFreeStorageBlockInArrayListSegment(psbOutStorageBlock, psaListCurrentHead, psaListOldHead))
		{
			bResult = true;
			break;
		}

		psaListOldHead = psaListCurrentHead;
		psaListCurrentHead = GetStorageArrayList();

		if (psaListOldHead == psaListCurrentHead)
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}

bool CTLSStorageInstance::FindFreeStorageBlockInArrayListSegment(CTLSStorageBlock *&psbOutStorageBlock, 
	CTLSStorageArray *psaListSegmentBegin, CTLSStorageArray *psaListSegmentEnd)
{
	OU_ASSERT(psaListSegmentBegin != psaListSegmentEnd);

	bool bResult;

	CTLSStorageArray *psaListSegmentCurrent = psaListSegmentBegin;

	while (true)
	{
		if (FindFreeStorageBlockFromArray(psbOutStorageBlock, psaListSegmentCurrent))
		{
			bResult = true;
			break;
		}

		psaListSegmentCurrent = psaListSegmentCurrent->GetNextArray();
		
		if (psaListSegmentCurrent == psaListSegmentEnd)
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}

bool CTLSStorageInstance::FindFreeStorageBlockFromArray(CTLSStorageBlock *&psbOutStorageBlock, 
	CTLSStorageArray *psaArrayInstance)
{
	tlsindextype iValueCount = GetValueCount();
	bool bIsManualCleanup = GetThreadManualCleanupFlag();

	return psaArrayInstance->FindFreeStorageBlock(psbOutStorageBlock, iValueCount, bIsManualCleanup);
}


void CTLSStorageInstance::AddStorageArrayToArrayList(CTLSStorageArray *psaStorageArray)
{
	while (true)
	{
		CTLSStorageArray *psaListCurrentHead = GetStorageArrayList();
		psaStorageArray->SetNextArray(psaListCurrentHead);

		if (TrySettingStorageArrayList(psaStorageArray, psaListCurrentHead))
		{
			break;
		}
	}
}


bool CTLSStorageInstance::AllocateStorageKey(HTLSKEYVALUE &hkvOutStorageKey, ESTORAGEINSTANCEKIND ikInstanceKind)
{
	bool bResult = false;

#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	DWORD dwTlsIndex = ::TlsAlloc();

	if (dwTlsIndex != TLS_OUT_OF_INDEXES)
	{
		hkvOutStorageKey = (HTLSKEYVALUE)(HTLSKEYVALUE::value_type)(size_t)dwTlsIndex;
		bResult = true;
	}
	

#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS
	
	pthread_key_t pkThreadKey;

	int iKeyCreationResult = pthread_key_create(&pkThreadKey, 
		(ikInstanceKind == SIK_AUTOCLEANUP) ? &CTLSStorageInstance::FreeStorageBlock_Callback_Automatic : &CTLSStorageInstance::FreeStorageBlock_Callback_Manual);
	if (iKeyCreationResult == EOK)
	{
		hkvOutStorageKey = (HTLSKEYVALUE)(HTLSKEYVALUE::value_type)(size_t)pkThreadKey;
		bResult = true;
	}
	
	
#endif // #if _OU_TARGET_OS == ...

	return bResult;
}

void CTLSStorageInstance::FreeStorageKey(const HTLSKEYVALUE &hkvStorageKey)
{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
	
	DWORD dwTlsIndex = (DWORD)(size_t)(HTLSKEYVALUE::value_type)hkvStorageKey;
	OU_ASSERT(dwTlsIndex != TLS_OUT_OF_INDEXES);

	BOOL bIndexFreeingResult = ::TlsFree(dwTlsIndex);
	OU_VERIFY(bIndexFreeingResult);
	
	
#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS
	
	pthread_key_t pkThreadKey = (pthread_key_t)(size_t)(HTLSKEYVALUE::value_type)hkvStorageKey;
	
	int iKeyDeletionResult = pthread_key_delete(pkThreadKey);
	OU_VERIFY(iKeyDeletionResult == EOK);
	
	
#endif // #if _OU_TARGET_OS == ...
}


#if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS

void CTLSStorageInstance::FreeStorageBlock_Callback_Automatic(void *pv_DataValue)
{
	if (pv_DataValue) // Just a precaution
	{
		CTLSStorageBlock *psbStorageBlock = (CTLSStorageBlock *)pv_DataValue;

		g_apsiStorageGlobalInstances[SIK_AUTOCLEANUP]->FreeStorageBlock(psbStorageBlock);
	}
}

void CTLSStorageInstance::FreeStorageBlock_Callback_Manual(void *pv_DataValue)
{
	if (pv_DataValue) // Just a precaution
	{
		CTLSStorageBlock *psbStorageBlock = (CTLSStorageBlock *)pv_DataValue;

		g_apsiStorageGlobalInstances[SIK_MANUALCLEANUP]->FreeStorageBlock(psbStorageBlock);
	}
}


#endif // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS


void CTLSStorageInstance::FreeStorageBlock(CTLSStorageBlock *psbStorageBlock)
{
	const int iValueCount = GetValueCount();
	
	CTLSStorageArray *psaArrayInstance = psbStorageBlock->GetHostArray();
	psaArrayInstance->FreeStorageBlockOnThreadExit(psbStorageBlock, iValueCount);
}


CTLSStorageArray *CTLSStorageInstance::AllocateStorageArray()
{
	const tlsindextype iValueCount = GetValueCount();

	return CTLSStorageArray::AllocateInstance(iValueCount);
}

void CTLSStorageInstance::FreeStorageArrayList(CTLSStorageArray *psaStorageArrayList)
{
	const tlsindextype iValueCount = GetValueCount();
	
	while (psaStorageArrayList)
	{
		CTLSStorageArray *psaStorageNextArray = psaStorageArrayList->GetNextArray();

		psaStorageArrayList->FreeInstance(iValueCount);
		
		psaStorageArrayList = psaStorageNextArray;
	}
}


//////////////////////////////////////////////////////////////////////////
// CThreadLocalStorage methods

bool CThreadLocalStorage::AllocateAndSetStorageValue(const HTLSKEYSELECTOR &hksKeySelector,
	tlsindextype iValueIndex, tlsvaluetype vValueData, CTLSValueDestructor fnValueDestructor)
{
	OU_ASSERT(OU_IN_SIZET_RANGE(DecodeInstanceKindFromKeySelector(hksKeySelector), SIK__MIN, SIK__MAX));

	bool bResult = false;
	
	do
	{
		ESTORAGEINSTANCEKIND ikInstanceKind = (ESTORAGEINSTANCEKIND)DecodeInstanceKindFromKeySelector(hksKeySelector);
		CTLSStorageInstance *psiStorageInstance = g_apsiStorageGlobalInstances[ikInstanceKind];

		CTLSStorageBlock *psbStorageBlock;

		if (!psiStorageInstance->FindFreeStorageBlock(psbStorageBlock))
		{
			break;
		}

		SetKeyStorageBlock(hksKeySelector, psbStorageBlock);

		psbStorageBlock->SetValueData(iValueIndex, vValueData);
		psbStorageBlock->SetValueDestructor(iValueIndex, fnValueDestructor);
	
		bResult = true;
	}
	while (false);
	
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
// CTLSInitialization methods

bool CTLSInitialization::InitializeTLSAPI(HTLSKEY &hskOutStorageKey, tlsindextype iValueCount,
	unsigned int uiInitializationFlags/*=0*/)
{
	OU_ASSERT(g_uiThreadLocalStorageInitializationCount != 0U - 1U);

	bool bResult = false;
	
	bool bAtomicAPIInitialized = false;

	do
	{
		const ESTORAGEINSTANCEKIND ikInstanceKind = (uiInitializationFlags & SIF_MANUAL_CLEANUP_ON_THREAD_EXIT) ? SIK_MANUALCLEANUP : SIK_AUTOCLEANUP;

		if (g_apsiStorageGlobalInstances[ikInstanceKind] == NULL) // Initialization/finalization must be called from main thread
		{
			if (!InitializeAtomicAPI())
			{
				break;
			}

			bAtomicAPIInitialized = true;

			if (!InitializeTLSAPIValidated(ikInstanceKind, iValueCount, uiInitializationFlags))
			{
				break;
			}

			const HTLSKEYVALUE &hkvStorageKey = g_apsiStorageGlobalInstances[ikInstanceKind]->RetrieveStorageKey();
			g_ahkvStorageGlobalKeyValues[ikInstanceKind] = hkvStorageKey;
		}

		++g_uiThreadLocalStorageInitializationCount;
	
		hskOutStorageKey = EncodeKeySelectorFromStorageKind(ikInstanceKind);
		OU_ASSERT(iValueCount == g_apsiStorageGlobalInstances[ikInstanceKind]->RetrieveValueCount());
		OU_ASSERT(uiInitializationFlags == g_apsiStorageGlobalInstances[ikInstanceKind]->RetrieveInitializationFlags());

		bResult = true;
	}
	while (false);
	
	if (!bResult)
	{
		if (bAtomicAPIInitialized)
		{
			FinalizeAtomicAPI();
		}
	}

	return bResult;
}

void CTLSInitialization::FinalizeTLSAPI()
{
	OU_ASSERT(g_uiThreadLocalStorageInitializationCount != 0U);

	ESTORAGEINSTANCEKIND ikInstanceKind = 
		(--g_uiThreadLocalStorageInitializationCount == 0U) ? SIK__MIN : SIK__MAX; // Initialization/finalization must be called from main thread
	for (; ikInstanceKind != SIK__MAX; ++ikInstanceKind) 
	{
		if (g_apsiStorageGlobalInstances[ikInstanceKind])
		{
			g_ahkvStorageGlobalKeyValues[ikInstanceKind] = 0;

			FinalizeTLSAPIValidated(ikInstanceKind);

			FinalizeAtomicAPI();
		}
	}
}


void CTLSInitialization::CleanupOnThreadExit()
{
	const ESTORAGEINSTANCEKIND ikInstanceKind = SIK_MANUALCLEANUP;
	CTLSStorageInstance *psiStorageInstance = g_apsiStorageGlobalInstances[ikInstanceKind];

	if (psiStorageInstance != NULL)
	{
		OU_ASSERT(psiStorageInstance->GetIsThreadManualCleanup());

		const HTLSKEYSELECTOR &hksKeySelector = EncodeKeySelectorFromStorageKind(ikInstanceKind);
		CTLSStorageBlock *psbStorageBlock = CThreadLocalStorage::GetKeyStorageBlock(hksKeySelector);
		
		if (psbStorageBlock)
		{
			psiStorageInstance->FreeStorageBlockOnThreadExit(psbStorageBlock);

			CThreadLocalStorage::SetKeyStorageBlock(hksKeySelector, NULL);
		}
	}
	else
	{
		OU_ASSERT(false); // The method is not supposed to be called if manual cleanup was not requested on initialization
	}
}


bool CTLSInitialization::InitializeTLSAPIValidated(unsigned int uiInstanceKind, 
	tlsindextype iValueCount, unsigned int uiInitializationFlags)
{
	OU_ASSERT(g_apsiStorageGlobalInstances[uiInstanceKind] == NULL);

	bool bResult = false;
	
	CTLSStorageInstance *psiStorageInstance;

	do
	{
		// Use static methods instead of constructor/destructor 
		// to avoid overloading operators new/delete and for 
		// uniformity with CTLSStorageArray class
		psiStorageInstance = CTLSStorageInstance::AllocateInstance(iValueCount, uiInitializationFlags);
		
		if (!psiStorageInstance)
		{
			break;
		}

		if (!psiStorageInstance->Init((ESTORAGEINSTANCEKIND)uiInstanceKind))
		{
			break;
		}

		g_apsiStorageGlobalInstances[uiInstanceKind] = psiStorageInstance;
	
		bResult = true;
	}
	while (false);
	
	if (!bResult)
	{
		if (psiStorageInstance)
		{
			psiStorageInstance->FreeInstance();
		}
	}

	return bResult;
}

void CTLSInitialization::FinalizeTLSAPIValidated(unsigned int uiInstanceKind)
{
	OU_ASSERT(g_apsiStorageGlobalInstances[uiInstanceKind] != NULL);

	g_apsiStorageGlobalInstances[uiInstanceKind]->FreeInstance();
	g_apsiStorageGlobalInstances[uiInstanceKind] = NULL;
}


END_NAMESPACE_OU();


#endif // #if _OU_FEATURE_SET >= _OU_FEATURE_SET_TLS

