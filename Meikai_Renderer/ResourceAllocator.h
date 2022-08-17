#pragma once
#include "MemDefine.h"
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include <deque>
using namespace Microsoft::WRL;
class DXApp;

class ResourceAllocator
{
	DXApp* m_App = nullptr;

public:
	struct Allocation
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
		size_t SIZE;

		template<typename T>
		void UpdateData(const T& data)
		{
			if (sizeof(T) > SIZE)
			{
				throw std::bad_alloc();
			}
			memcpy(CPU, &data, sizeof(T));
		}
	};

	explicit ResourceAllocator(DXApp* appPtr, size_t pageSize = _2MB);
	virtual ~ResourceAllocator();

	size_t GetPageSize() const { return m_PageSize; }

	Allocation Allocate(size_t sizeInBytes, size_t alignment); 
	void Reset();

private:
	struct Page
	{
		Page(DXApp* appPtr, size_t sizeInBytes);
		~Page();

		bool HasSpace(size_t sizeInBytes, size_t alignment) const;
		Allocation Allocate(size_t sizeInBytes, size_t alignment);
		void Reset();
	private:
		ComPtr<ID3D12Resource> m_Resource;
		void* m_CPUPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

		size_t m_PageSize;
		size_t m_Offset;
	};

	using PagePool = std::deque<std::shared_ptr<Page>>;
	std::shared_ptr<Page> RequestPage();

	PagePool m_PagePool;
	PagePool m_AvailablePages;

	std::shared_ptr<Page> m_CurrentPage;

	size_t m_PageSize;
};

