// Copyright 2019 ayumax. All Rights Reserved.

#include "CaptureMachine.h"
#include "Engine/Texture2D.h"
#include "Async/Async.h"
#include "Internationalization/Regex.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "../Private/Utils/WCWorkerThread.h"

#if PLATFORM_WINDOWS
#include <dwmapi.h>
#endif

#if ENGINE_MAJOR_VERSION >= 5
#define FTicker FTSTicker
#endif

UCaptureMachine::UCaptureMachine()
{
}

void UCaptureMachine::Start()
{
#if PLATFORM_WINDOWS
	try
	{
		m_TargetWindow = nullptr;
		m_TargetMonitor = nullptr;
		if (Properties.Target == ECaptureTarget::Window)
		{
			::EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
				{
					UCaptureMachine* my = reinterpret_cast<UCaptureMachine*>(lParam);
					return my->FindTargetWindow(hwnd);
				}, reinterpret_cast<LPARAM>(this));

			if (!m_TargetWindow)
			{
				return;
			}
		}
		else
		{
			struct FEnumData 
			{
				int index = 0;
				UCaptureMachine* Machine = nullptr;
			} EnumData = {0, this};

			::EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL
				{
					FEnumData* Data = reinterpret_cast<FEnumData*>(dwData);
					if (Data->index == Data->Machine->Properties.CaptureTargetIndex)
					{
						Data->Machine->m_TargetMonitor = hMonitor;
						return false;
					}
					Data->index++;
					return true;

				}, reinterpret_cast<LPARAM>(&EnumData));
			if (!m_TargetMonitor)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to find montior: %d"), EnumData.index);
				return;
			}
		}

		m_WinrtItem = CreateCaptureItem();
		if (!m_WinrtItem)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to CreateCaptureItem()"));
			return;
		}
		m_WinrtItem.Closed({ this, &UCaptureMachine::OnTargetClosed });

		m_WinrtDevice = CreateDevice();
		if (!m_WinrtDevice)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to CreateDevice()"));
			return;
		}

		// StartCapture
		m_WinrtFramePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
			m_WinrtDevice,
			winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			2,
			m_WinrtItem.Size());

		m_WinrtSession = m_WinrtFramePool.CreateCaptureSession(m_WinrtItem);
		m_WinrtSession.StartCapture();

		if (TickHandle.IsValid())
		{
			FTicker::GetCoreTicker().RemoveTicker(TickHandle);
			TickHandle.Reset();
		}
		TickDelegate = FTickerDelegate::CreateUObject(this, &UCaptureMachine::Tick);
		TickHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate, 1.0f / (float)Properties.FrameRate);
	}
	catch (const winrt::hresult_error& e)
	{
		const int code = e.code();
		const winrt::hstring message = e.message();
		UE_LOG(LogTemp, Error, TEXT("winrt::hresult_error %d %s"), code, message.c_str());
	}
#endif
}

void UCaptureMachine::Close()
{
#if PLATFORM_WINDOWS
	if (TickHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

	if (TextureTarget)
	{
		TextureTarget->ReleaseResource();
		TextureTarget = nullptr;
	}

	if (m_WinrtFramePool)
	{
		m_WinrtRevoker.revoke();
	}

	if (m_WinrtSession)
	{
		m_WinrtSession.Close();
		m_WinrtSession = nullptr;
	}

	if (m_WinrtFramePool)
	{
		m_WinrtFramePool.Close();
		m_WinrtFramePool = nullptr;
	}

	m_WinrtDevice = nullptr;
	m_WinrtItem = nullptr;
#endif

}

bool UCaptureMachine::Tick(float deltaTime)
{
#if PLATFORM_WINDOWS
	OnFrameArrived(m_WinrtFramePool, nullptr);
#endif
	return true;
}

#if PLATFORM_WINDOWS
void UCaptureMachine::OnTargetClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem Item, winrt::Windows::Foundation::IInspectable Inspectable)
{
	ChangeTexture.Broadcast(nullptr);
}


winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice UCaptureMachine::CreateDevice()
{
	try
	{
		UINT createDeviceFlags = D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		winrt::com_ptr<ID3D11Device> d3dDevice;
		winrt::check_hresult(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			d3dDevice.put(),
			nullptr,
			nullptr));

		winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice.as<IDXGIDevice>();
		winrt::com_ptr<::IInspectable> device;
		winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), device.put()));

		return device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
	}
	catch (const winrt::hresult_error& e)
	{
		const int code = e.code();
		const winrt::hstring message = e.message();
		UE_LOG(LogTemp, Error, TEXT("winrt::hresult_error %d %s"), code, message.c_str());
		return nullptr;
	}
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem UCaptureMachine::CreateCaptureItem()
{
	try
	{
		winrt::Windows::Foundation::IActivationFactory factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
		winrt::impl::com_ref<::IGraphicsCaptureItemInterop> interop = factory.as<::IGraphicsCaptureItemInterop>();
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
		if (m_TargetWindow)
		{
			winrt::check_hresult(interop->CreateForWindow(m_TargetWindow, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item))));
		}
		else
		{
			winrt::check_hresult(interop->CreateForMonitor(m_TargetMonitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item))));
		}
		return item;
	}
	catch (const winrt::hresult_error& e)
	{
		const int code = e.code();
		const winrt::hstring message = e.message();
		UE_LOG(LogTemp, Error, TEXT("winrt::hresult_error %d %s"), code, message.c_str());
		return nullptr;
	}

}

void UCaptureMachine::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
	try
	{
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame = sender.TryGetNextFrame();
		if (!frame)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to TryGetNextFrame() on OnFrameArrived"));
			return;
		}

		winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface surface = frame.Surface();
		if (!surface)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to Surface() on OnFrameArrived"));
			return;
		}

		winrt::impl::com_ref<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> access =
			surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
		winrt::check_hresult(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), m_WinrtTexture.put_void()));

		winrt::Windows::Graphics::SizeInt32 Size = frame.ContentSize();
		if ((m_WinrtSize.Height != Size.Height) || (m_WinrtSize.Width != Size.Width))
		{
			m_WinrtSize = Size;
			m_WinrtFramePool.Recreate(
				m_WinrtDevice,
				winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
				2,
				m_WinrtSize);
		}

		UpdateTextureFromID3D11Texture2D(m_WinrtTexture);
		ChangeTexture.Broadcast(TextureTarget);
	}
	catch (const winrt::hresult_error& e)
	{
		const int code = e.code();
		const winrt::hstring message = e.message();
		UE_LOG(LogTemp, Error, TEXT("winrt::hresult_error %d %s"), code, message.c_str());
	}
}

void UCaptureMachine::UpdateTextureFromID3D11Texture2D(winrt::com_ptr<ID3D11Texture2D> Texture)
{
	try
	{
		// First verify that we can map the texture
		D3D11_TEXTURE2D_DESC desc;
		Texture->GetDesc(&desc);

		// Get the device context
		winrt::com_ptr<ID3D11Device> d3dDevice;
		Texture->GetDevice(d3dDevice.put());
		winrt::com_ptr<ID3D11DeviceContext> d3dContext;
		d3dDevice->GetImmediateContext(d3dContext.put());

		// map the texture
		winrt::com_ptr<ID3D11Texture2D> mappedTexture;
		D3D11_MAPPED_SUBRESOURCE mapInfo;
		mapInfo.RowPitch;
		HRESULT hr = d3dContext->Map(
			Texture.get(),
			0,  // Subresource
			D3D11_MAP_READ,
			0,  // MapFlags
			&mapInfo);
		if (FAILED(hr))
		{
			if (hr == E_INVALIDARG)
			{
				D3D11_TEXTURE2D_DESC desc2;
				desc2.Width = desc.Width;
				desc2.Height = desc.Height;
				desc2.MipLevels = desc.MipLevels;
				desc2.ArraySize = desc.ArraySize;
				desc2.Format = desc.Format;
				desc2.SampleDesc = desc.SampleDesc;
				desc2.Usage = D3D11_USAGE_STAGING;
				desc2.BindFlags = 0;
				desc2.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc2.MiscFlags = 0;

				winrt::com_ptr<ID3D11Texture2D> stagingTexture;
				winrt::check_hresult(d3dDevice->CreateTexture2D(&desc2, nullptr, stagingTexture.put()));

				// copy the texture to a staging resource
				d3dContext->CopyResource(stagingTexture.get(), Texture.get());

				// now, map the staging resource
				winrt::check_hresult(d3dContext->Map(
					stagingTexture.get(),
					0,
					D3D11_MAP_READ,
					0,
					&mapInfo));

				mappedTexture = std::move(stagingTexture);
			}
			else
			{
				winrt::check_hresult(hr);
			}
		}
		else
		{
			mappedTexture = Texture;
		}

		if (!m_WinrtTexture) return;

		UINT Width = desc.Width;
		UINT Height = desc.Height;

		if ((m_Width != Width) ||
			(m_Height != Height))
		{
			m_Width = Width;
			m_Height = Height;
			ReCreateTexture();
		}

		auto Region = new FUpdateTextureRegion2D(0, 0, 0, 0, m_Width, m_Height);
		TextureTarget->UpdateTextureRegions(0, 1, Region, mapInfo.RowPitch, 4, (uint8*)mapInfo.pData);
	}
	catch (const winrt::hresult_error& e)
	{
		const int code = e.code();
		const winrt::hstring message = e.message();
		UE_LOG(LogTemp, Error, TEXT("winrt::hresult_error %d %s"), code, message.c_str());
	}
}
#endif

UTexture2D* UCaptureMachine::CreateTexture()
{
	return nullptr;
}

bool UCaptureMachine::FindTargetWindow(HWND hWnd)
{
#if PLATFORM_WINDOWS
	__wchar_t windowTitle[1024];
	GetWindowText(hWnd, windowTitle, 1024);
	FString title(windowTitle);

	if (title.IsEmpty()) return true;

	bool isMatch = false;

	switch (Properties.TitleMatchingWindowSearch)
	{
	case ETitleMatchingWindowSearch::PerfectMatch:
		isMatch = title.Equals(Properties.CaptureTargetTitle, ESearchCase::IgnoreCase);
		break;

	case ETitleMatchingWindowSearch::ForwardMatch:
		isMatch = title.StartsWith(Properties.CaptureTargetTitle, ESearchCase::IgnoreCase);
		break;

	case ETitleMatchingWindowSearch::PartialMatch:
		isMatch = title.Contains(Properties.CaptureTargetTitle, ESearchCase::IgnoreCase);
		break;

	case ETitleMatchingWindowSearch::BackwardMatch:
		isMatch = title.EndsWith(Properties.CaptureTargetTitle, ESearchCase::IgnoreCase);
		break;

	case ETitleMatchingWindowSearch::RegularExpression:
		{
			const FRegexPattern pattern = FRegexPattern(Properties.CaptureTargetTitle);
			FRegexMatcher matcher(pattern, title);

			isMatch = matcher.FindNext();
		}
		break;
	}

	if (isMatch)
	{
		m_TargetWindow = hWnd;
		return false;
	}
#endif

	return true;
}

void UCaptureMachine::ReCreateTexture()
{
#if PLATFORM_WINDOWS
	if (m_Height == 0 || m_Width == 0)
	{
		TextureTarget = nullptr;
		return;
	}

	TextureTarget = UTexture2D::CreateTransient(m_Width, m_Height, PF_B8G8R8A8);
	TextureTarget->UpdateResource();
#endif
}

FWindowStatus UCaptureMachine::GetCurrentWindowStatus()
{
	FWindowStatus status;
	status.top = 0;
	status.bottom = 0;
	status.left = 0;
	status.right = 0;

	if (Properties.Target == ECaptureTarget::Monitor)
	{
		if (!m_TargetMonitor) return status;

		MONITORINFOEX Info;
		Info.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(m_TargetMonitor, &Info);
		FString title(Info.szDevice);

		UE_LOG(LogTemp, Log, TEXT("%s"), *title);
		UE_LOG(LogTemp, Log, TEXT("top=%d"), Info.rcWork.top);
		UE_LOG(LogTemp, Log, TEXT("bottom=%d"), Info.rcWork.bottom);
		UE_LOG(LogTemp, Log, TEXT("left=%d"), Info.rcWork.left);
		UE_LOG(LogTemp, Log, TEXT("right=%d"), Info.rcWork.right);
		status.title = title;
		status.top = Info.rcWork.top;
		status.bottom = Info.rcWork.bottom;
		status.left = Info.rcWork.left;
		status.right = Info.rcWork.right;
		return status;
	}
	else
	{
		if (!m_TargetWindow) return status;

		__wchar_t windowTitle[1024];
		GetWindowText(m_TargetWindow, windowTitle, 1024);
		FString title(windowTitle);

		RECT rect;
		::GetWindowRect(m_TargetWindow, &rect);

		LONG_PTR style = GetWindowLongPtr(m_TargetWindow, GWL_STYLE);
		//UE_LOG(LogTemp, Log, TEXT("%s"), windowTitle);
		//UE_LOG(LogTemp, Log, TEXT("top=%d"), rect.top);
		//UE_LOG(LogTemp, Log, TEXT("bottom=%d"), rect.bottom);
		//UE_LOG(LogTemp, Log, TEXT("left=%d"), rect.left);
		//UE_LOG(LogTemp, Log, TEXT("right=%d"), rect.right);
		status.title = title;
		status.top = rect.top;
		status.bottom = rect.bottom;
		status.left = rect.left;
		status.right = rect.right;
		return status;
	}
}

void UCaptureMachine::SetActiveWindow()
{
	if (m_TargetWindow)
	{
		ShowWindow(m_TargetWindow, SW_RESTORE);
		ShowWindow(m_TargetWindow, SW_SHOW);
		SetForegroundWindow(m_TargetWindow);
	}
}