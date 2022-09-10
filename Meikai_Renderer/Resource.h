#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <string>
#include <memory>

using namespace Microsoft::WRL;
class DXApp;
class Resource
{
public:
	explicit Resource(DXApp* appPtr, const std::wstring& name = L"");
	explicit Resource(DXApp* appPtr, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr,
		const std::wstring& name = L"");
	explicit Resource(DXApp* appPtr, ComPtr<ID3D12Resource> resource, const std::wstring& name = L"");

	Resource(const Resource& copy);
	Resource(Resource&& copy);

	Resource& operator=(const Resource& other);
	Resource& operator=(Resource&& other) noexcept;

	virtual ~Resource();

	bool IsValid() const;
	ComPtr<ID3D12Resource> GetResource() const;
	D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const;

	virtual void SetD3D12Resource(ComPtr<ID3D12Resource> newResource, const D3D12_CLEAR_VALUE* clearValue = nullptr);
	
	virtual void Reset();

	void SetName(const std::wstring& name);

	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
	bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;

protected:
	DXApp* mApp;

protected:
	ComPtr<ID3D12Resource> mResource;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT mFormatSupport;
	std::unique_ptr<D3D12_CLEAR_VALUE> mClearValue;
	std::wstring mResourceName;

private:
	void CheckFeatureSupport();
};

