#include "ResourceAllocator.h"
#include "DXUtil.h"
#include "DXApp.h"

ResourceAllocator::ResourceAllocator(DXApp* appPtr, size_t pageSize)
	:m_App(appPtr), m_PageSize(pageSize)
{

}

ResourceAllocator::~ResourceAllocator()
{

}

ResourceAllocator::Allocation ResourceAllocator::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_PageSize)
	{
		throw std::bad_alloc();
	}

	if (m_CurrentPage == false || m_CurrentPage->HasSpace(sizeInBytes, alignment) == false)
	{
		m_CurrentPage = RequestPage();
	}

	return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

std::shared_ptr<ResourceAllocator::Page> ResourceAllocator::RequestPage()
{
	std::shared_ptr<Page> page;
	if (m_AvailablePages.empty())
	{
		page = std::make_shared<Page>(m_App, m_PageSize);
		m_PagePool.push_back(page);
	}
	else
	{
		page = m_AvailablePages.front();
		m_AvailablePages.pop_front();
	}

	return page;
}

void ResourceAllocator::Reset()
{
	m_CurrentPage = nullptr;
	m_AvailablePages = m_PagePool;

	for (auto page : m_AvailablePages)
	{
		page->Reset();
	}
}

ResourceAllocator::Page::Page(DXApp* appPtr, size_t sizeInBytes)
	:m_PageSize(sizeInBytes),
	m_Offset(0),
	m_CPUPtr(nullptr),
	m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	auto device = appPtr->GetDevice();

	ThrowIfFailed(device->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_PageSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)
	));
	m_GPUPtr = m_Resource->GetGPUVirtualAddress();
	m_Resource->Map(0, nullptr, &m_CPUPtr);
}

ResourceAllocator::Page::~Page()
{
	m_Resource->Unmap(0, nullptr);
	m_CPUPtr = nullptr;
	m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool ResourceAllocator::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = Math::AlignUp(m_Offset, alignment);

	return (alignedOffset + alignedSize) <= m_PageSize;
}

ResourceAllocator::Allocation ResourceAllocator::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (HasSpace(sizeInBytes, alignment) == false)
	{
		throw std::bad_alloc();
	}

	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	m_Offset = Math::AlignUp(m_Offset, alignment);

	Allocation allocation;
	allocation.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
	allocation.GPU = m_GPUPtr + m_Offset;
	allocation.SIZE = alignedSize;

	m_Offset += alignedSize;

	return allocation;
}

void ResourceAllocator::Page::Reset()
{
	m_Offset = 0;
}
