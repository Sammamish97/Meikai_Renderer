#pragma once
#include "DXApp.h"

class Demo : public DXApp
{
public:
	Demo(HINSTANCE hInstance);
	~Demo();

	bool Initialize() override;
protected:
	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

private:
	void CreateDsvDescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE Demo::DepthStencilView() const;

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
};

