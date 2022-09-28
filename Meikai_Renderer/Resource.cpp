#include "Resource.h"
#include "DXApp.h"
#include "DXUtil.h"

Resource::Resource(DXApp* appPtr, const std::wstring& name)
	: mApp(appPtr),
	mResourceName(name), 
	mResource(nullptr), 
	mFormatSupport({}),
	mClearValue(nullptr)
{

}

Resource::Resource(DXApp* appPtr, const D3D12_RESOURCE_DESC& resourceDesc,
	const D3D12_CLEAR_VALUE* clearValue,
	const std::wstring& name)
	: mApp(appPtr)
{
	if (clearValue)
	{
		mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
	}
	auto device = mApp->GetDevice();
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		mClearValue.get(),
		IID_PPV_ARGS(mResource.GetAddressOf())))

	CheckFeatureSupport();
	SetName(name);
}

Resource::Resource(DXApp* appPtr, ComPtr<ID3D12Resource> resource, const std::wstring& name)
	: mApp(appPtr), mResource(resource), mFormatSupport({})
{
	CheckFeatureSupport();
	SetName(name);
}

Resource::Resource(const Resource& copy)
	: mApp(copy.mApp),
	mResource(copy.mResource),
	mFormatSupport(copy.mFormatSupport),
	mResourceName(copy.mResourceName)
{
	if(copy.mClearValue)
	{
		mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*copy.mClearValue);
	}
}

Resource::Resource(Resource&& copy)
	: mApp(copy.mApp),
	mResource(std::move(copy.mResource)),
	mFormatSupport(copy.mFormatSupport),
	mResourceName(std::move(copy.mResourceName)),
	mClearValue(std::move(copy.mClearValue))
{
}

Resource& Resource::operator=(const Resource& other)
{
	if (this != &other)
	{
		mApp = other.mApp;
		mResource = other.mResource;
		mFormatSupport = other.mFormatSupport;
		mResourceName = other.mResourceName;
		if (other.mClearValue)
		{
			mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*other.mClearValue);
		}
	}
	return *this;
}
Resource& Resource::operator=(Resource&& other) noexcept
{
	if (this != &other)
	{
		mApp = other.mApp;
		mResource = std::move(other.mResource);
		mFormatSupport = other.mFormatSupport;
		mResourceName = std::move(other.mResourceName);
		mClearValue = std::move(other.mClearValue);

		other.Reset();
	}
	return *this;
}

Resource::~Resource()
{

}

bool Resource::IsValid() const
{
	return mResource != nullptr;
}

ComPtr<ID3D12Resource> Resource::GetResource() const
{
	return mResource;
}

D3D12_RESOURCE_DESC Resource::GetD3D12ResourceDesc() const
{
	D3D12_RESOURCE_DESC resDesc = {};
	if (mResource)
	{
		resDesc = mResource->GetDesc();
	}

	return resDesc;
}

void Resource::SetD3D12Resource(ComPtr<ID3D12Resource> newResource, const D3D12_CLEAR_VALUE* clearValue)
{
	mResource = newResource;
	if (mClearValue)
	{
		mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
	}
	else
	{
		mClearValue.reset();
	}
	CheckFeatureSupport();
	SetName(mResourceName);
}

void Resource::Reset()
{
	mResource.Reset();
	mFormatSupport = {};
	mClearValue.reset();
	mResourceName.clear();
}

void Resource::SetName(const std::wstring& name)
{
	mResourceName = name;
	if (mResource && mResourceName.empty() == false)
	{
		mResource->SetName(mResourceName.c_str());
	}
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
	return (mFormatSupport.Support1 & formatSupport) != 0;
}
bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
	return (mFormatSupport.Support2 & formatSupport) != 0;
}

void Resource::CheckFeatureSupport()
{
	if (mResource)
	{
		auto desc = mResource->GetDesc();
		auto device = mApp->GetDevice();

		mFormatSupport.Format = desc.Format;
		ThrowIfFailed(device->CheckFeatureSupport(
			D3D12_FEATURE_FORMAT_SUPPORT,
			&mFormatSupport,
			sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
	}
	else
	{
		mFormatSupport = {};
	}
}