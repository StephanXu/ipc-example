#pragma once
#include <comdef.h>

#ifndef B_SHAREMEMORY_H
#define B_SHAREMEMORY_H

class MemorySharer {
public:
	PVOID CreateMemory(const _bstr_t MemID, const LONG size) {
		m_hMapFile = CreateFileMapping(
			(HANDLE)0xFFFFFFFF,
			NULL,
			PAGE_READWRITE,
			0,
			size,
			MemID);
		this->pData = MapViewOfFile(
			m_hMapFile,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			0,
			0);
		return pData;
	}
	PVOID GetMemory(const _bstr_t MemID) {
		m_hMapFile = OpenFileMapping(
			FILE_MAP_WRITE,
			FALSE,
			MemID);
		pData = MapViewOfFile(
			m_hMapFile,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			0,
			0);
		return pData;
	}
	PVOID pData;
	HANDLE m_hMapFile;
};

#endif