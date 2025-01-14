// Copyright 2019 ayumax. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CaptureMachineProperties.h"
#include "Containers/Ticker.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include <WinUser.h>
#else
using HBITMAP = void*;
using HDC = void*;
using HWND = void*;
#endif

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <d3d11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "CaptureMachine.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCaptureMachineChangeTexture, UTexture2D*, NewTexture);

USTRUCT(BlueprintType)
struct FWindowStatus {

	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = WindowCapture2D)
	FString title;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = WindowCapture2D)
	int top;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = WindowCapture2D)
	int bottom;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = WindowCapture2D)
	int left;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = WindowCapture2D)
	int right;
};

UCLASS(BlueprintType, Blueprintable)
class WINDOWCAPTURE2D_API UCaptureMachine : public UObject
{
	GENERATED_BODY()
	
public:	
	UCaptureMachine();

	virtual void Start();
	virtual void Close();

	UFUNCTION(BlueprintPure, Category = WindowCapture2D)
	UTexture2D* CreateTexture();

	UFUNCTION(BlueprintCallable, Category = WindowCapture2D)
	FWindowStatus GetCurrentWindowStatus();

	UFUNCTION(BlueprintCallable, Category = WindowCapture2D)
	void SetActiveWindow();

protected:
	bool FindTargetWindow(HWND hWnd);
	void ReCreateTexture();
	bool Tick(float deltaTime);

#if PLATFORM_WINDOWS
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateCaptureItem();
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice CreateDevice();
	void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args);
	void OnTargetClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem Item, winrt::Windows::Foundation::IInspectable Inspectable);
	void UpdateTextureFromID3D11Texture2D(winrt::com_ptr<ID3D11Texture2D> texture);
#endif


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WindowCapture2D)
	FCaptureMachineProperties Properties;

	UPROPERTY(BlueprintReadOnly, Category = SceneCapture)
	class UTexture2D* TextureTarget;

	UPROPERTY(BlueprintAssignable, Category = SceneCapture)
	FCaptureMachineChangeTexture ChangeTexture;

private:
	FIntVector2D m_WindowSize;
	FIntVector2D m_OriginalWindowSize;
	FIntVector2D m_WindowOffset;

	FTickerDelegate TickDelegate;
#if ENGINE_MAJOR_VERSION >= 5
	FTSTicker::FDelegateHandle TickHandle;
#else
	FDelegateHandle TickHandle;
#endif

#if PLATFORM_WINDOWS
	HWND m_TargetWindow = nullptr;
	HMONITOR m_TargetMonitor = nullptr;

	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_WinrtDevice = nullptr;
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_WinrtItem = nullptr;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_WinrtFramePool = nullptr;
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_WinrtSession = nullptr;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_WinrtRevoker;
	winrt::Windows::Graphics::SizeInt32 m_WinrtSize = { 0, 0 };
	winrt::com_ptr<ID3D11Texture2D> m_WinrtTexture = nullptr;
	UINT m_Width = 0;
	UINT m_Height = 0;
#endif
};
