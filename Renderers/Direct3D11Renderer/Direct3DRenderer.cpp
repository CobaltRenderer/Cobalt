// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderer.h"
#include "Direct3DDataArray.h"
#include "Direct3DDataArrayOutput.h"
#include "Direct3DDefaultState.h"
#include "Direct3DFrameBuffer.h"
#include "Direct3DFrameBufferOutput.h"
#include "Direct3DIndexBuffer.h"
#include "Direct3DProgramNode.h"
#include "Direct3DRenderPassNode.h"
#include "Direct3DRenderableNode.h"
#include "Direct3DShaderProgram.h"
#include "Direct3DStateBuffer.h"
#include "Direct3DStateBufferLayout.h"
#include "Direct3DStateGroupNode.h"
#include "Direct3DTexelArray.h"
#include "Direct3DTexelArrayOutput.h"
#include "Direct3DTextureBuffer1D.h"
#include "Direct3DTextureBuffer1DArray.h"
#include "Direct3DTextureBuffer2D.h"
#include "Direct3DTextureBuffer2DArray.h"
#include "Direct3DTextureBuffer3D.h"
#include "Direct3DTextureBufferCube.h"
#include "Direct3DTextureBufferCubeArray.h"
#include "Direct3DTextureSampler1D.h"
#include "Direct3DTextureSampler1DArray.h"
#include "Direct3DTextureSampler2D.h"
#include "Direct3DTextureSampler2DArray.h"
#include "Direct3DTextureSampler3D.h"
#include "Direct3DTextureSamplerCube.h"
#include "Direct3DTextureSamplerCubeArray.h"
#include "Direct3DTransferBatch.h"
#include "Direct3DVertexBuffer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/RenderMarkers.h>
#include <functional>
#include <iostream>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DRenderer::Direct3DRenderer(cobalt::logging::ILogger::unique_ptr log, Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter2> adapter, const Marshal::In<std::set<IGraphicsDevice::Feature>>& enabledFeatures, const Marshal::In<std::set<Options>>& enabledOptions)
: _buildIndex(0), _drawIndex(1), _dxgiFactory(std::move(dxgiFactory)), _adapter(std::move(adapter)), _enabledOptions(enabledOptions)
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderer::Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags)
{
	_enableDebugLogging = (_enabledOptions.find(Options::EnableDebugLogging) != _enabledOptions.end());

	// Retrieve the target adapter to create the device for
	IDXGIAdapter2* targetAdapter = _adapter.Get();
	bool hasTargetAdapter = (targetAdapter != nullptr);

	// Enable DXGI debug logging if requested
	if (_enableDebugLogging)
	{
		HRESULT dxgiGetDebugInterface1Return = DXGIGetDebugInterface1(0, IID_PPV_ARGS(_dxgiInfoQueue.GetAddressOf()));
		if (FAILED(dxgiGetDebugInterface1Return))
		{
			_log->Error("DXGIGetDebugInterface1 failed with error code {0}. API debug logging will not be redirected to internal log system.", dxgiGetDebugInterface1Return);
		}
#ifdef _DEBUG
		else
		{
			// Enable break on severe errors for debug builds
			_dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		}
#endif
	}

	// Attempt to initialize Direct3D. Note that as Direct3D guarantees VS/GS/PSSetConstantBuffers1 is available with
	// hardware feature level 10 or higher, we don't need the 11.1 feature level here. We do need the Direct3D 11.1 API,
	// which is only fully supported on Windows 8, however the 11.1 hardware feature level is optional. We request it
	// here in preference, but fall back to 11.0 in case that it's not available. This gives us safe support for NVidia
	// and AMD graphics hardware back to 2010, and Intel graphics hardware back to 2013. Note that there is an update
	// for Windows 7 that brings partial Direct3D 11.1 support. With some effort, it looks likely that Windows 7 can be
	// supported if that is desirable. See the following article for more information:
	// https://docs.microsoft.com/en-gb/windows/desktop/direct3darticles/platform-update-for-windows-7
	// Be aware that if support for Windows 7 is added, we may need to be aware of a known bug with emulation of
	// VS/GS/PSSetConstantBuffers1. See the following article for more information:
	// https://docs.microsoft.com/en-us/windows/desktop/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-vssetconstantbuffers1
	// This appears to not apply to us if we're targeting at least the 11.0 feature level on Windows 8 or higher, but we
	// may be exposed to this issue on the Windows 7 platform. A way of detecting and working around the issue is given
	// in the referenced article.
	D3D_DRIVER_TYPE driverType = (targetAdapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN);
	auto targetFeatureLevels = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
	auto targetFeatureLevelCount = (UINT)targetFeatureLevels.size();
	D3D_FEATURE_LEVEL selectedFeatureLevel;
	UINT createDeviceFlags = 0;
	if (_enableDebugLogging)
	{
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	Microsoft::WRL::ComPtr<ID3D11Device> deviceTemp;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContextTemp;
	HRESULT d3d11CreateDeviceReturn = D3D11CreateDevice(targetAdapter, driverType, nullptr, createDeviceFlags, targetFeatureLevels.begin(), targetFeatureLevelCount, D3D11_SDK_VERSION, &deviceTemp, &selectedFeatureLevel, &deviceContextTemp);
	if (FAILED(d3d11CreateDeviceReturn))
	{
		_log->Error("D3D11CreateDevice failed with error code {0}", d3d11CreateDeviceReturn);
		return false;
	}

	// Write a log message identifying the hardware feature level we selected
	auto selectedFeatureLevelName = (selectedFeatureLevel == D3D_FEATURE_LEVEL_11_1) ? std::wstring(L"11.1") : std::wstring(L"11.0");
	_log->Info("Created Direct3D device at feature level {0}", selectedFeatureLevelName);

	// Obtain the Direct3D 11.1 interfaces
	HRESULT castDeviceReturn = deviceTemp.As(&_device);
	if (FAILED(castDeviceReturn))
	{
		_log->Error("Failed to cast ID3D11Device to ID3D11Device1 with error code {0}", castDeviceReturn);
		return false;
	}
	deviceTemp.Reset();
	HRESULT castDeviceContextReturn = deviceContextTemp.As(&_deviceContext);
	if (FAILED(castDeviceContextReturn))
	{
		_log->Error("Failed to cast ID3D11DeviceContext to ID3D11DeviceContext1 with error code {0}", castDeviceContextReturn);
		return false;
	}
	deviceContextTemp.Reset();

	// Route debug messages through our logging system and filter out noise which isn't relevant for our usage
	if (_enableDebugLogging)
	{
		HRESULT queryInfoQueueReturn = _device.As(&_debugInfoQueue);
		if (FAILED(queryInfoQueueReturn))
		{
			_log->Warning("Failed to retrieve ID3D11InfoQueue with error code {0}. API debug logging will not be redirected to internal log system.", queryInfoQueueReturn);
		}
		else
		{
			// Enable break on severe errors for debug builds
#ifdef _DEBUG
			_debugInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
#endif

			// Filter out messages we don't consider useful
			auto messageFilterList = {
			  D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET,
			};
			D3D11_INFO_QUEUE_FILTER messageFilter = {};
			messageFilter.DenyList.NumIDs = (UINT)messageFilterList.size();
			messageFilter.DenyList.pIDList = const_cast<D3D11_MESSAGE_ID*>(messageFilterList.begin());
			HRESULT addStorageFilterEntriesReturn = _debugInfoQueue->AddStorageFilterEntries(&messageFilter);
			if (FAILED(addStorageFilterEntriesReturn))
			{
				_log->Warning("Failed to add debug message filter with error code {0}", addStorageFilterEntriesReturn);
			}
		}
	}

	// If we didn't have a target adapter supplied, obtain the adapter and DXGI factory from device.
	if (!hasTargetAdapter)
	{
		Microsoft::WRL::ComPtr<IDXGIDevice2> dxgiDevice;
		HRESULT castDxgiDeviceReturn = _device.As(&dxgiDevice);
		if (FAILED(castDxgiDeviceReturn))
		{
			_log->Error("Failed to cast ID3D11Device1 to IDXGIDevice2 with error code {0}", castDxgiDeviceReturn);
			return false;
		}
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
		HRESULT getAdapterReturn = dxgiDevice->GetAdapter(&adapter);
		if (FAILED(getAdapterReturn))
		{
			_log->Error("GetAdapter failed with error code {0}", getAdapterReturn);
			return false;
		}
		HRESULT castAdapterReturn = adapter.As(&_adapter);
		if (FAILED(castAdapterReturn))
		{
			_log->Error("Failed to cast IDXGIAdapter to IDXGIAdapter2 with error code {0}", castAdapterReturn);
			return false;
		}
		HRESULT getParentReturn = adapter->GetParent(IID_PPV_ARGS(&_dxgiFactory));
		if (FAILED(getParentReturn))
		{
			_log->Error("GetParent failed with error code {0}", getParentReturn);
			return false;
		}
	}

	// Check if render markers feature is enabled
	if (_enabledOptions.find(Options::EnableRenderMarkers) != _enabledOptions.end())
	{
		// Obtain annotation object for adding scopes and markers
		HRESULT getAnnotationReturn = _deviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), &_renderAnnotation);
		if (FAILED(getAnnotationReturn) || (_renderAnnotation.Get() == nullptr))
		{
			_log->Warning("Failed to get ID3DUserDefinedAnnotation with error code {0}. Render markers will be disabled", getAnnotationReturn);
		}
		else
		{
			_useRenderMarkers = true;
		}
	}

	// Start the render worker thread
	_renderThreadActive = true;
	_frameAdvanceInProgress = false;
	_buildingInProgress = true;
	_drawingInProgress = false;
	_buildToDrawRequestPending = false;
	_renderRequestPending = false;
	_swapRequestPending = false;
	_deleteLastDrawResourcesRequestPending = false;
	_earlyDeleteNextDrawResourcesRequestPending = false;
	std::thread workerThread(std::bind(std::mem_fn(&Direct3DRenderer::RenderThread), this));
	workerThread.detach();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::Delete()
{
	// Terminate the render worker thread
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_renderThreadActive)
	{
		_renderThreadActive = false;
		_notifyRenderThreadTaskPending.notify_all();
		_notifyRenderThreadStopped.wait(lock);
	}
	lock.unlock();

	// Delete any objects which are pending deletion
	PerformDeleteLastDrawResourcesOperation();
	PerformDeleteNextDrawResourcesOperation();

	// Release all remaining objects except our device and device context objects
	_renderAnnotation.Reset();
	_dxgiFactory.Reset();

	// Reset the device context state, and perform a flush to trigger immediate deletion of all objects pending
	// deletion, then release the device context. Unfortunately we'll still be left with some remaining references from
	// the device context itself, and any objects it was referencing internally, but the number should be relatively
	// small, and the "Refcount" for these objects should correctly show zero.
	if (_deviceContext.Get() != nullptr)
	{
		_deviceContext->ClearState();
		_deviceContext->Flush();
		_deviceContext.Reset();
	}

	// Write debug output for leaked object references if requested. Note that as this is the Direct3D 11 API report, we
	// expect this to generate a warning about the device object still being live, as we can't release it yet, since
	// it's generating this report. For this reason, we only do this live object reporting for debug builds. The
	// following DXGI live object report covers the same thing, and won't have this live device object reporting issue.
#ifdef _DEBUG
	if (_enableDebugLogging && (_device.Get() != nullptr))
	{
		Microsoft::WRL::ComPtr<ID3D11Debug> debugInterface;
		if (!FAILED(_device.As(&debugInterface)))
		{
			debugInterface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		}
	}
#endif

	// Release our device object
	_device.Reset();

	// Process any remaining debug messages, and release the info queue.
	if (_enableDebugLogging && (_debugInfoQueue.Get() != nullptr))
	{
		ProcessPendingDebugMessages();
		_debugInfoQueue.Reset();
		_dxgiInfoQueue.Reset();
	}

	// If debug logging is enabled, generate a live objects report from DXGI.
	if (_enableDebugLogging)
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
		}
	}

	// Delete this object
	delete this;
}

//----------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------
const char* Direct3DRenderer::DebugMessageCategoryToString(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category)
{
	switch (category)
	{
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS:
		return "Miscellaneous";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION:
		return "Initialization";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP:
		return "Cleanup";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION:
		return "Compilation";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION:
		return "StateCreation";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING:
		return "StateSetting";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING:
		return "StateGetting";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:
		return "ResourceManipulation";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION:
		return "Execution";
	case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER:
		return "Shader";
	}
	return nullptr;
}

//----------------------------------------------------------------------------------------
const char* Direct3DRenderer::DebugMessageCategoryToString(D3D11_MESSAGE_CATEGORY category)
{
	switch (category)
	{
	case D3D11_MESSAGE_CATEGORY_APPLICATION_DEFINED:
		return "Application";
	case D3D11_MESSAGE_CATEGORY_MISCELLANEOUS:
		return "Miscellaneous";
	case D3D11_MESSAGE_CATEGORY_INITIALIZATION:
		return "Initialization";
	case D3D11_MESSAGE_CATEGORY_CLEANUP:
		return "Cleanup";
	case D3D11_MESSAGE_CATEGORY_COMPILATION:
		return "Compilation";
	case D3D11_MESSAGE_CATEGORY_STATE_CREATION:
		return "StateCreation";
	case D3D11_MESSAGE_CATEGORY_STATE_SETTING:
		return "StateSetting";
	case D3D11_MESSAGE_CATEGORY_STATE_GETTING:
		return "StateGetting";
	case D3D11_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:
		return "ResourceManipulation";
	case D3D11_MESSAGE_CATEGORY_EXECUTION:
		return "Execution";
	case D3D11_MESSAGE_CATEGORY_SHADER:
		return "Shader";
	}
	return nullptr;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::DebugMessageCallbackInternal(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category, DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity, DXGI_INFO_QUEUE_MESSAGE_ID messageId, LPCSTR description) const
{
	// Build the category string and message description
	const char* categoryString = DebugMessageCategoryToString(category);
	std::string categoryStringTemp;
	if (categoryString == nullptr)
	{
		categoryStringTemp = "Unknown:" + std::to_string((uint32_t)category);
		categoryString = categoryStringTemp.c_str();
	}
	const char* messageDescription = (description != nullptr ? description : "");

	// Forward the message into our logging system
	switch (severity)
	{
	case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION:
	case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:
		_log->Error("DXGI Error [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:
		_log->Warning("DXGI Warning [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:
		_log->Info("DXGI Info [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE:
		_log->Debug("DXGI Debug [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	default:
		_log->Debug("DXGI ({3}) [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription, (uint32_t)severity);
		break;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::DebugMessageCallbackInternal(D3D11_MESSAGE_CATEGORY category, D3D11_MESSAGE_SEVERITY severity, D3D11_MESSAGE_ID messageId, LPCSTR description) const
{
	// Build the category string and message description
	const char* categoryString = DebugMessageCategoryToString(category);
	std::string categoryStringTemp;
	if (categoryString == nullptr)
	{
		categoryStringTemp = "Unknown:" + std::to_string((uint32_t)category);
		categoryString = categoryStringTemp.c_str();
	}
	const char* messageDescription = (description != nullptr ? description : "");

	// Forward the message into our logging system
	switch (severity)
	{
	case D3D11_MESSAGE_SEVERITY_CORRUPTION:
	case D3D11_MESSAGE_SEVERITY_ERROR:
		_log->Error("Direct3D 11 Error [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D11_MESSAGE_SEVERITY_WARNING:
		_log->Warning("Direct3D 11 Warning [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D11_MESSAGE_SEVERITY_INFO:
		_log->Info("Direct3D 11 Info [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D11_MESSAGE_SEVERITY_MESSAGE:
		_log->Debug("Direct3D 11 Debug [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	default:
		_log->Debug("Direct3D 11 ({3}) [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription, (uint32_t)severity);
		break;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::ProcessPendingDebugMessages()
{
	// Process the DXGI debug message queue
	if (_dxgiInfoQueue != nullptr)
	{
		// Retrieve the number of pending debug log messages
		UINT64 messageCount = _dxgiInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilters(DXGI_DEBUG_DXGI);

		// Forward every debug message to our logging system
		for (UINT64 i = 0; i < messageCount; ++i)
		{
			// Retrieve the required size of the buffer to hold the next message
			SIZE_T messageLength = 0;
			HRESULT getMessageReturn = _dxgiInfoQueue->GetMessage(DXGI_DEBUG_DXGI, i, nullptr, &messageLength);
			if (FAILED(getMessageReturn))
			{
				_log->Error("IDXGIInfoQueue::GetMessage failed with error code {0}", getMessageReturn);
				continue;
			}

			// Resize our scratch buffer to be large enough to hold the message
			_debugMessageBuffer.resize(messageLength);
			auto* message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(_debugMessageBuffer.data());

			// Retrieve the next message
			getMessageReturn = _dxgiInfoQueue->GetMessage(DXGI_DEBUG_DXGI, i, message, &messageLength);
			if (FAILED(getMessageReturn))
			{
				_log->Error("IDXGIInfoQueue::GetMessage failed with error code {0}", getMessageReturn);
				continue;
			}

			// Forward the message to our logging system
			DebugMessageCallbackInternal(message->Category, message->Severity, message->ID, message->pDescription);
		}

		// Clear the list of processed log messages
		_dxgiInfoQueue->ClearStoredMessages(DXGI_DEBUG_DXGI);
	}

	// Process the Direct3D debug message queue
	if (_debugInfoQueue != nullptr)
	{
		// Retrieve the number of pending debug log messages
		UINT64 messageCount = _debugInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

		// Forward every debug message to our logging system
		for (UINT64 i = 0; i < messageCount; ++i)
		{
			// Retrieve the required size of the buffer to hold the next message
			SIZE_T messageLength = 0;
			HRESULT getMessageReturn = _debugInfoQueue->GetMessage(i, nullptr, &messageLength);
			if (FAILED(getMessageReturn))
			{
				_log->Error("ID3D11InfoQueue::GetMessage failed with error code {0}", getMessageReturn);
				continue;
			}

			// Resize our scratch buffer to be large enough to hold the message
			_debugMessageBuffer.resize(messageLength);
			auto* message = reinterpret_cast<D3D11_MESSAGE*>(_debugMessageBuffer.data());

			// Retrieve the next message
			getMessageReturn = _debugInfoQueue->GetMessage(i, message, &messageLength);
			if (FAILED(getMessageReturn))
			{
				_log->Error("ID3D11InfoQueue::GetMessage failed with error code {0}", getMessageReturn);
				continue;
			}

			// Forward the message to our logging system
			DebugMessageCallbackInternal(message->Category, message->Severity, message->ID, message->pDescription);
		}

		// Clear the list of processed log messages
		_debugInfoQueue->ClearStoredMessages();
	}
}

//----------------------------------------------------------------------------------------
// Geometry buffer methods
//----------------------------------------------------------------------------------------
IVertexBuffer* Direct3DRenderer::CreateVertexBufferInternal()
{
	return new Direct3DVertexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IIndexBuffer* Direct3DRenderer::CreateIndexBufferInternal()
{
	return new Direct3DIndexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image buffer methods
//----------------------------------------------------------------------------------------
ITextureBuffer1D* Direct3DRenderer::CreateTextureBuffer1DInternal()
{
	return new Direct3DTextureBuffer1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2D* Direct3DRenderer::CreateTextureBuffer2DInternal()
{
	return new Direct3DTextureBuffer2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer3D* Direct3DRenderer::CreateTextureBuffer3DInternal()
{
	return new Direct3DTextureBuffer3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCube* Direct3DRenderer::CreateTextureBufferCubeInternal()
{
	return new Direct3DTextureBufferCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer1DArray* Direct3DRenderer::CreateTextureBuffer1DArrayInternal()
{
	return new Direct3DTextureBuffer1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2DArray* Direct3DRenderer::CreateTextureBuffer2DArrayInternal()
{
	return new Direct3DTextureBuffer2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCubeArray* Direct3DRenderer::CreateTextureBufferCubeArrayInternal()
{
	return new Direct3DTextureBufferCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image sampler methods
//----------------------------------------------------------------------------------------
ITextureSampler1D* Direct3DRenderer::CreateTextureSampler1DInternal()
{
	return new Direct3DTextureSampler1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2D* Direct3DRenderer::CreateTextureSampler2DInternal()
{
	return new Direct3DTextureSampler2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler3D* Direct3DRenderer::CreateTextureSampler3DInternal()
{
	return new Direct3DTextureSampler3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCube* Direct3DRenderer::CreateTextureSamplerCubeInternal()
{
	return new Direct3DTextureSamplerCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler1DArray* Direct3DRenderer::CreateTextureSampler1DArrayInternal()
{
	return new Direct3DTextureSampler1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2DArray* Direct3DRenderer::CreateTextureSampler2DArrayInternal()
{
	return new Direct3DTextureSampler2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCubeArray* Direct3DRenderer::CreateTextureSamplerCubeArrayInternal()
{
	return new Direct3DTextureSamplerCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Data array methods
//----------------------------------------------------------------------------------------
IDataArray* Direct3DRenderer::CreateDataArrayInternal()
{
	return new Direct3DDataArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IDataArrayOutput* Direct3DRenderer::CreateDataArrayOutputInternal()
{
	return new Direct3DDataArrayOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITexelArray* Direct3DRenderer::CreateTexelArrayInternal()
{
	return new Direct3DTexelArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITexelArrayOutput* Direct3DRenderer::CreateTexelArrayOutputInternal()
{
	return new Direct3DTexelArrayOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Batch methods
//----------------------------------------------------------------------------------------
ITransferBatch* Direct3DRenderer::CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming)
{
	return new Direct3DTransferBatch(_log.get(), this, startTiming, endTiming);
}

//----------------------------------------------------------------------------------------
// Frame buffer methods
//----------------------------------------------------------------------------------------
IFrameBuffer* Direct3DRenderer::CreateFrameBufferInternal()
{
	return new Direct3DFrameBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IFrameBufferOutput* Direct3DRenderer::CreateFrameBufferOutputInternal()
{
	return new Direct3DFrameBufferOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
IStateBuffer* Direct3DRenderer::CreateStateBufferInternal()
{
	return new Direct3DStateBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateBufferLayout* Direct3DRenderer::CreateStateBufferLayoutInternal()
{
	return new Direct3DStateBufferLayout(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Render tree node methods
//----------------------------------------------------------------------------------------
IRenderPassNode* Direct3DRenderer::CreateRenderPassNodeInternal()
{
	return new Direct3DRenderPassNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IProgramNode* Direct3DRenderer::CreateProgramNodeInternal()
{
	return new Direct3DProgramNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateGroupNode* Direct3DRenderer::CreateStateGroupNodeInternal()
{
	return new Direct3DStateGroupNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IRenderableNode* Direct3DRenderer::CreateRenderableNodeInternal()
{
	return new Direct3DRenderableNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IDefaultState* Direct3DRenderer::CreateDefaultStateInternal()
{
	return new Direct3DDefaultState(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Program methods
//----------------------------------------------------------------------------------------
IShaderProgram* Direct3DRenderer::CreateShaderProgramInternal()
{
	return new Direct3DShaderProgram(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Scene content methods
//----------------------------------------------------------------------------------------
void Direct3DRenderer::SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry{};
			renderPassEntry.renderPassNode = KnownDynamicCast<Direct3DRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<Direct3DRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry{};
			renderPassEntry.renderPassNode = KnownDynamicCast<Direct3DRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<Direct3DRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::RemoveAllRenderPasses()
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].renderPasses.clear();
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void Direct3DRenderer::StartNewFrame()
{
	// Ensure a frame advance operation isn't already in progress
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_frameAdvanceInProgress)
	{
		_log->Error("StartNewFrame was called when a previous call was still in progress");
		return;
	}
	_frameAdvanceInProgress = true;

	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. We need to do this as we're about to overwrite state data from the last frame.
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Submit a build to draw request to the render thread if required, and wait for it to be processed. This will
	// also signal that a drawing operation is now in progress.
	if (_buildingInProgress)
	{
		_buildToDrawRequestPending = true;
		_notifyRenderThreadTaskPending.notify_all();
		while (_buildToDrawRequestPending)
		{
			_notifyRenderThreadTaskComplete.wait(lock);
		}
	}

	// Flag that we're beginning to build a new frame
	_buildingInProgress = true;

	// Signal that a frame advance operation is no longer in progress
	_frameAdvanceInProgress = false;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::WaitForDrawComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::WaitForOutputCaptureComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Flag to any framebuffer outputs that captured data in the frame we just completed a draw for that the application
	// is allowed to read the captured output from the live draw state. We can do this safely at this point as the
	// application has explicitly synchronized with the completion of framebuffer output capture, which it should be
	// noted is not necessarily complete when the draw process itself is complete. In the case of our renderer here it
	// currently is the same, so we use the same synchronization point.
	for (auto* entry : _capturedFramebufferOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
	for (auto* entry : _capturedDataArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
	for (auto* entry : _capturedTexelArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::WaitForDeferredDeletionComplete() const
{
	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. Additionally, we also request early deletion of resources associated with the next frame. We
	// can do this safely, since we're synchronizing with the end of the previous frame first. This creates a safe
	// point at which external window resources can be freed without a new frame being sent to the renderer.
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_earlyDeleteNextDrawResourcesRequestPending = true;
	_notifyRenderThreadTaskPending.notify_all();
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AddCurrentFrameBufferOutput(Direct3DFrameBufferOutput* frameBufferOutput)
{
	_capturedFramebufferOutputsInCurrentFrame.push_back(frameBufferOutput);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AddCurrentDataArrayOutput(Direct3DDataArrayOutput* resourceBufferOutput)
{
	_capturedDataArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AddCurrentDataArray(Direct3DDataArray* resourceBuffer)
{
	_boundDataArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AddCurrentTexelArrayOutput(Direct3DTexelArrayOutput* resourceBufferOutput)
{
	_capturedTexelArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AddCurrentTexelArray(Direct3DTexelArray* resourceBuffer)
{
	_boundTexelArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::RenderThread()
{
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	bool done = false;
	while (!done)
	{
		// Wait for work to be requested or the thread to be requested to terminate
		if (!_buildToDrawRequestPending && !_renderRequestPending && !_swapRequestPending && !_deleteLastDrawResourcesRequestPending && !_earlyDeleteNextDrawResourcesRequestPending)
		{
			// If the render thread has not already been instructed to stop, suspend this thread until a render task is
			// issued or this thread is instructed to stop.
			if (_renderThreadActive)
			{
				_notifyRenderThreadTaskPending.wait(lock);
			}

			// If the render thread has been suspended, flag that we need to exit the render loop.
			done = !_renderThreadActive;

			// Begin the loop again
			continue;
		}

		// Action the next pending render request
		if (_deleteLastDrawResourcesRequestPending)
		{
			lock.unlock();
			PerformDeleteLastDrawResourcesOperation();
			lock.lock();
			_deleteLastDrawResourcesRequestPending = false;
		}
		else if (_buildToDrawRequestPending)
		{
			_drawingInProgress = true;
			lock.unlock();
			PerformPrepareBuildOperation();
			lock.lock();
			_buildToDrawRequestPending = false;
			_renderRequestPending = true;
		}
		else if (_renderRequestPending)
		{
			lock.unlock();
			if (UseLegacyRenderingMethod())
			{
				PerformLegacyRenderOperation();
			}
			else
			{
				PerformRenderOperation();
			}
			lock.lock();
			_renderRequestPending = false;
			_swapRequestPending = true;
		}
		else if (_swapRequestPending)
		{
			lock.unlock();
			PerformSwapOperation();
			lock.lock();
			_swapRequestPending = false;
			_drawingInProgress = false;
			_deleteLastDrawResourcesRequestPending = true;
		}
		else if (_earlyDeleteNextDrawResourcesRequestPending)
		{
			// Note that there's a priority order here. This request must be processed after all the above.
			lock.unlock();
			PerformDeleteNextDrawResourcesOperation();
			lock.lock();
			_earlyDeleteNextDrawResourcesRequestPending = false;
		}

		// Notify any waiting threads that a render thread task has completed
		_notifyRenderThreadTaskComplete.notify_all();
	}

	// Notify any waiting threads that the render thread has now terminated
	_notifyRenderThreadStopped.notify_all();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformPrepareBuildOperation()
{
	// Transition our build state to our draw state
	std::swap(_buildIndex, _drawIndex);
	_state[_buildIndex].renderPasses = _state[_drawIndex].renderPasses;
	_state[_buildIndex].migrateStatePendingObjects.clear();
	_state[_buildIndex].bufferUpdatePendingObjects.clear();
	_state[_buildIndex].bufferTransferPendingObjects.clear();

	// Transfer build state into draw state in our scene tree
	for (const RenderPassEntry& renderPassEntry : _state[_drawIndex].renderPasses)
	{
		renderPassEntry.renderPassNode->MigrateBuildStateToDrawState();
	}

	// Transfer build state info draw state in our modified standalone objects
	for (const auto& entry : _state[_drawIndex].migrateStatePendingObjects)
	{
		std::visit([](auto* object) { object->MigrateBuildStateToDrawState(); }, entry);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformRenderOperation()
{
	// Push a render marker if required
	if (_useRenderMarkers)
	{
		_renderAnnotation->BeginEvent(L"Setup");
	}

	// Initiate any pending data transfer operations within GPU memory
	ID3D11Device1* device = _device.Get();
	ID3D11DeviceContext1* context = _deviceContext.Get();
	for (const auto& entry : _state[_drawIndex].bufferTransferPendingObjects)
	{
		std::visit([device, context](auto* object) { object->CompletePendingDataTransfers(device, context); }, entry);
	}

	// Initiate any pending data transfer operations from CPU to GPU memory
	for (const auto& entry : _state[_drawIndex].bufferUpdatePendingObjects)
	{
		std::visit([device, context](auto* object) { object->CompletePendingDataWrites(device, context); }, entry);
	}

	// Since it's possible for the same shader program to be used more than once in the render tree, and there's some
	// data which is tracked per shader, we do a quick pre-pass to the program node level here to flag all active shader
	// programs in the scene as uninitialized. Although alternate implementations could avoid this extra partial scene
	// traversal, on the balance of things this was considered to be the simplest and possibly fastest approach in the
	// real world. Alternate methods should be profiled to determine effectiveness if changes are made here.
	const std::vector<RenderPassEntry>& renderPassEntries = _state[_drawIndex].renderPasses;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		Direct3DRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const Direct3DRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			Direct3DShaderProgram* shaderProgram = childNodeEntry.node->GetShaderProgram();
			shaderProgram->ResetGlobalConstantBufferState();
		}
	}

	// Traverse the render tree to fill our global shared state buffer. This technique is the current state of the art
	// in preparing per-frame constant data buffers. See the following article from NVidia for more information:
	// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		Direct3DRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// Process each child program node
		const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const Direct3DRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			Direct3DProgramNode* programNode = childNodeEntry.node;
			Direct3DShaderProgram* shaderProgram = nullptr;
			Direct3DShaderProgram::GlobalConstantBufferBuildingSession constantBufferBuildingSession{};
			const std::vector<Direct3DStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			for (Direct3DStateGroupNode* groupNode : groupNodes)
			{
				// If this state group node is empty, skip it.
				const std::vector<Direct3DRenderableNode*>& renderableNodes = groupNode->GetChildNodes();
				bool isComputeTask = groupNode->HasComputeTask();
				if (!isComputeTask && renderableNodes.empty())
				{
					continue;
				}

				// If we haven't latched the shader program yet, do it now. We defer this process to minimize work if
				// all the state nodes for a given program node are empty.
				if (shaderProgram == nullptr)
				{
					// Retrieve the shader program
					shaderProgram = programNode->GetShaderProgram();

					// Apply constant state values for the program node
					const auto& constantValueEntries = programNode->GetConstantValueEntries();
					shaderProgram->RestoreGlobalConstantBufferBaseline();
					for (IConstantStateValueInfo* constantValue : constantValueEntries)
					{
						constantValue->ApplyValue(shaderProgram);
					}

					// Apply default state values from the program node binding at the render pass level
					Direct3DDefaultState* defaultState = childNodeEntry.defaultState;
					if (defaultState != nullptr)
					{
						const auto& stateValueEntriesForDefaultState = defaultState->GetValueEntries();
						for (const auto& stateValue : stateValueEntriesForDefaultState)
						{
							stateValue->ApplyValue(shaderProgram);
						}
					}

					// Push the current global constant buffer state so we can restore it later
					shaderProgram->PushGlobalConstantBufferState();

					// Start a new global constant buffer building session
					shaderProgram->BeginGlobalConstantBufferBuildingSession(constantBufferBuildingSession, context);
				}

				// If this state group node sets global constant state, load it now.
				const auto& stateValueEntriesForGroupNode = groupNode->GetValueEntries();
				bool hasGroupStateValues = !stateValueEntriesForGroupNode.empty();
				if (hasGroupStateValues)
				{
					// Apply state values for this state group node
					for (const auto& stateValue : stateValueEntriesForGroupNode)
					{
						stateValue->ApplyValue(shaderProgram);
					}

					// Now that we've applied state values, push the current global constant buffer state so we can
					// restore it later.
					shaderProgram->PushGlobalConstantBufferState();
				}

				// If this group node is performing a compute task, associate the current constant buffer state with the
				// group node.
				if (isComputeTask)
				{
					// Bind the generated global constant data to the target renderable
					shaderProgram->GenerateGlobalConstantBufferBindings(constantBufferBuildingSession, groupNode->GetGlobalConstantBufferBindingInfo(), context);
				}

				// Bind global constant state to each contained renderable node
				for (Direct3DRenderableNode* renderableNode : renderableNodes)
				{
					// Apply any state values for this renderable node
					const auto& stateValueEntriesForRenderableNode = renderableNode->GetValueEntries();
					bool hasRenderableStateValues = !stateValueEntriesForRenderableNode.empty();
					if (hasRenderableStateValues)
					{
						for (const auto& stateValue : stateValueEntriesForRenderableNode)
						{
							stateValue->ApplyValue(shaderProgram);
						}
					}

					// Bind the generated global constant data to the target renderable
					shaderProgram->GenerateGlobalConstantBufferBindings(constantBufferBuildingSession, renderableNode->GetGlobalConstantBufferBindingInfo(), context);

					// If we changed state values for this renderable, restore it to the pushed state baseline.
					if (hasRenderableStateValues)
					{
						shaderProgram->RestoreGlobalConstantBufferBaseline();
					}
				}

				// If we loaded state from the state group node, pop the global constant buffer state.
				if (hasGroupStateValues)
				{
					shaderProgram->PopGlobalConstantBufferState();
				}
			}

			// If we built a global constant buffer for this program node, complete the session now, and pop the global
			// constant buffer state.
			if (shaderProgram != nullptr)
			{
				shaderProgram->CompleteGlobalConstantBufferBuildingSession(constantBufferBuildingSession, context);
				shaderProgram->PopGlobalConstantBufferState();
			}
		}
	}

	// Pop a render marker if required
	if (_useRenderMarkers)
	{
		_renderAnnotation->EndEvent();
	}

	// Traverse the render tree and perform our draw operations
	_boundWindowFramebuffers.clear();
	_boundTextureFramebuffers.clear();
	_boundDataArrays.clear();
	_boundTexelArrays.clear();
	Direct3DShaderProgram* currentShaderProgram = nullptr;
	Direct3DFrameBuffer* currentFramebuffer = nullptr;
	size_t boundRenderTargetCount = 0;
	bool isComputeShader = false;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		Direct3DRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// Push a render marker if required
		if (_useRenderMarkers)
		{
			_renderAnnotation->BeginEvent(renderPassNode->DebugName().c_str());
		}

		// Bind the framebuffer for this render pass
		Direct3DFrameBuffer* newFramebuffer = renderPassNode->GetFrameBuffer();
		if (newFramebuffer != currentFramebuffer)
		{
			boundRenderTargetCount = 0;
			if (newFramebuffer != nullptr)
			{
				newFramebuffer->BindFrameBuffer(device, context, _dxgiFactory.Get());
				if (newFramebuffer->IsBoundToWindow())
				{
					_boundWindowFramebuffers.push_back(newFramebuffer);
				}
				else
				{
					_boundTextureFramebuffers.push_back(newFramebuffer);
				}
				boundRenderTargetCount = newFramebuffer->GetRenderTargetViews().size();
			}
			currentFramebuffer = newFramebuffer;
		}

		// Apply fixed state from this render pass node
		renderPassNode->ApplyFixedState(device, context);

		// Render each child node
		const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const Direct3DRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			// If there are no child nodes in this program node, skip it.
			Direct3DProgramNode* programNode = childNodeEntry.node;
			const std::vector<Direct3DStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			if (groupNodes.empty())
			{
				continue;
			}

			// Push a render marker if required
			if (_useRenderMarkers)
			{
				_renderAnnotation->BeginEvent(programNode->DebugName().c_str());
			}

			// Bind the shader program for this program node
			Direct3DShaderProgram* shaderProgram = programNode->GetShaderProgram();
			if (shaderProgram != currentShaderProgram)
			{
				if (currentShaderProgram != nullptr)
				{
					currentShaderProgram->UnbindAllShaderResources(context);
				}
				shaderProgram->BindShaderProgram(context, false);
				currentShaderProgram = shaderProgram;
				isComputeShader = currentShaderProgram->IsComputeShader();
			}

			// Bind any textures or state buffer entries set as defaults at the render pass level
			Direct3DDefaultState* defaultState = childNodeEntry.defaultState;
			const std::vector<ITextureBindingInfo*>* defaultTextureEntries = nullptr;
			const std::vector<ISamplerBindingInfo*>* defaultSamplerEntries = nullptr;
			const std::vector<StateBufferBindingInfo*>* defaultStateBufferEntries = nullptr;
			const std::vector<ResourceArrayBindingInfo*>* defaultResourceBufferEntries = nullptr;
			if (defaultState != nullptr)
			{
				defaultTextureEntries = &defaultState->GetTextureEntries();
				defaultSamplerEntries = &defaultState->GetSamplerEntries();
				defaultStateBufferEntries = &defaultState->GetStateBufferEntries();
				defaultResourceBufferEntries = &defaultState->GetResourceBufferEntries();
			}
			bool hasDefaultTextureEntries = (defaultTextureEntries != nullptr) && !defaultTextureEntries->empty();
			bool hasDefaultSamplerEntries = (defaultSamplerEntries != nullptr) && !defaultSamplerEntries->empty();
			bool hasDefaultStateBufferEntries = (defaultStateBufferEntries != nullptr) && !defaultStateBufferEntries->empty();
			bool hasDefaultResourceBufferEntries = (defaultResourceBufferEntries != nullptr) && !defaultResourceBufferEntries->empty();
			if (hasDefaultTextureEntries)
			{
				BindTextures(*defaultTextureEntries, device, context, shaderProgram);
			}
			if (hasDefaultSamplerEntries)
			{
				BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
			}
			if (hasDefaultStateBufferEntries)
			{
				BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
			}
			if (hasDefaultResourceBufferEntries)
			{
				BindResourceArrays(defaultResourceBufferEntries, nullptr, nullptr, device, context, shaderProgram, true, false, false, boundRenderTargetCount, isComputeShader);
			}

			// Render each child node
			for (Direct3DStateGroupNode* groupNode : groupNodes)
			{
				// If this state group node is empty, skip it.
				bool isComputeTask = groupNode->HasComputeTask();
				const std::vector<Direct3DRenderableNode*>& renderableNodes = groupNode->GetChildNodes();
				if (!isComputeTask && renderableNodes.empty())
				{
					continue;
				}

				// Push a render marker if required
				if (_useRenderMarkers)
				{
					_renderAnnotation->BeginEvent(groupNode->DebugName().c_str());
				}

				// Apply fixed state associated with this state group node
				groupNode->ApplyFixedState(device, context, currentFramebuffer);

				// Bind any textures or state buffer entries set on this state node
				const auto& textureEntriesForGroupNode = groupNode->GetTextureEntries();
				const auto& samplerEntriesForGroupNode = groupNode->GetSamplerEntries();
				const auto& stateBufferEntriesForGroupNode = groupNode->GetStateBufferEntries();
				const auto& resourceBufferEntriesForGroupNode = groupNode->GetResourceBufferEntries();
				bool hasTextureEntriesForStateGroupNode = !textureEntriesForGroupNode.empty();
				bool hasSamplerEntriesForStateGroupNode = !samplerEntriesForGroupNode.empty();
				bool hasStateBufferEntriesForStateGroupNode = !stateBufferEntriesForGroupNode.empty();
				bool hasResourceBufferEntriesForStateGroupNode = !resourceBufferEntriesForGroupNode.empty();
				if (hasTextureEntriesForStateGroupNode)
				{
					if (hasDefaultTextureEntries)
					{
						BindTextures(*defaultTextureEntries, device, context, shaderProgram);
					}
					BindTextures(textureEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasSamplerEntriesForStateGroupNode)
				{
					if (hasDefaultSamplerEntries)
					{
						BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
					}
					BindSamplers(samplerEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasStateBufferEntriesForStateGroupNode)
				{
					if (hasDefaultStateBufferEntries)
					{
						BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
					}
					BindStateBuffers(stateBufferEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasResourceBufferEntriesForStateGroupNode)
				{
					BindResourceArrays(defaultResourceBufferEntries, &resourceBufferEntriesForGroupNode, nullptr, device, context, shaderProgram, false, true, false, boundRenderTargetCount, isComputeTask);
				}

				// If this group node is performing a compute task, execute it now.
				if (isComputeTask)
				{
					// Attach the required global constant buffer entries
					shaderProgram->ApplyGlobalConstantBufferBindings(groupNode->GetGlobalConstantBufferBindingInfo(), context);

					// Perform the compute task
					auto threadGroupCounts = groupNode->GetComputeThreadGroupCounts();
					context->Dispatch(threadGroupCounts.X(), threadGroupCounts.Y(), threadGroupCounts.Z());
				}

				// Render each child node
				for (Direct3DRenderableNode* renderableNode : renderableNodes)
				{
					// Push a render marker if required
					if (_useRenderMarkers)
					{
						_renderAnnotation->SetMarker(renderableNode->DebugName().c_str());
					}

					// Bind any textures or state buffer entries set on this renderable node
					const auto& textureEntriesForRenderableNode = renderableNode->GetTextureEntries();
					const auto& samplerEntriesForRenderableNode = renderableNode->GetSamplerEntries();
					const auto& stateBufferEntriesForRenderableNode = renderableNode->GetStateBufferEntries();
					const auto& resourceBufferEntriesForRenderableNode = renderableNode->GetResourceBufferEntries();
					bool hasTextureEntries = !textureEntriesForRenderableNode.empty();
					bool hasSamplerEntries = !samplerEntriesForRenderableNode.empty();
					bool hasStateBufferEntries = !stateBufferEntriesForRenderableNode.empty();
					bool hasResourceBufferEntries = !resourceBufferEntriesForRenderableNode.empty();
					if (hasTextureEntries)
					{
						BindTextures(textureEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasSamplerEntries)
					{
						BindSamplers(samplerEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasStateBufferEntries)
					{
						BindStateBuffers(stateBufferEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasResourceBufferEntries)
					{
						BindResourceArrays(defaultResourceBufferEntries, (hasResourceBufferEntriesForStateGroupNode ? &resourceBufferEntriesForGroupNode : nullptr), &resourceBufferEntriesForRenderableNode, device, context, shaderProgram, false, false, true, boundRenderTargetCount, false);
					}

					// Attach the required global constant buffer entries
					shaderProgram->ApplyGlobalConstantBufferBindings(renderableNode->GetGlobalConstantBufferBindingInfo(), context);

					// If the renderable node has been setup to use indirect multi-draw with a draw count retrieved from
					// GPU memory, retrieve the value now. This will be inefficient, as it involves a CPU round trip and
					// will therefore cause a pipeline stall. We accept this here to allow Direct3D 11 to emulate
					// indirect multi-draw, which it doesn't natively support.
					if (renderableNode->UsesIndirectMultiDrawWithVariableCount())
					{
						// Obtain the settings on which buffer to read the draw counter from, and the location of the
						// value within the buffer.
						Direct3DDataArray* drawCountBuffer;
						bool drawCountFromCounter;
						size_t drawCountBufferOffsetInBytes;
						renderableNode->GetIndirectMultiDrawCountSourceBufferInfo(drawCountBuffer, drawCountFromCounter, drawCountBufferOffsetInBytes);

						// Read the draw counter value from the buffer
						UINT drawCount;
						if (drawCountFromCounter)
						{
							drawCount = drawCountBuffer->GetCurrentCounterValue(device, context);
						}
						else
						{
							drawCountBuffer->GetCurrentBufferData(device, context, drawCountBufferOffsetInBytes, &drawCount, sizeof(drawCount));
						}

						// Update the current draw count value in the renderable node
						renderableNode->SetCurrentIndirectDrawCount(drawCount);
					}

					// Draw this renderable object
					renderableNode->Draw(shaderProgram, context);

					// If we bound any textures or state buffer entries, and the state group node provided some initial
					// bindings of its own, restore the bindings that were set on the state group node now.
					if (hasTextureEntries)
					{
						if (hasDefaultTextureEntries)
						{
							BindTextures(*defaultTextureEntries, device, context, shaderProgram);
						}
						if (hasTextureEntriesForStateGroupNode)
						{
							BindTextures(textureEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasSamplerEntries)
					{
						if (hasDefaultSamplerEntries)
						{
							BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
						}
						if (hasSamplerEntriesForStateGroupNode)
						{
							BindSamplers(samplerEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasStateBufferEntries)
					{
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
						}
						if (hasStateBufferEntriesForStateGroupNode)
						{
							BindStateBuffers(stateBufferEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasResourceBufferEntries)
					{
						if (hasResourceBufferEntriesForStateGroupNode)
						{
							BindResourceArrays(defaultResourceBufferEntries, &resourceBufferEntriesForGroupNode, nullptr, device, context, shaderProgram, false, false, false, boundRenderTargetCount, false);
						}
						else if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(defaultResourceBufferEntries, nullptr, nullptr, device, context, shaderProgram, false, false, false, boundRenderTargetCount, false);
						}
					}
				}

				// Pop a render marker if required
				if (_useRenderMarkers)
				{
					_renderAnnotation->EndEvent();
				}
			}

			// Pop a render marker if required
			if (_useRenderMarkers)
			{
				_renderAnnotation->EndEvent();
			}
		}

		// Perform any unbind operations for the render pass node. Currently this is limited to multisample framebuffer
		// resolution.
		renderPassNode->PerformUnbindOperations(context);

		// Unbind any current shader resources, to prevent warnings about active shader resources being bound to
		// framebuffer output targets.
		if (currentShaderProgram != nullptr)
		{
			currentShaderProgram->UnbindAllShaderResources(context);
			currentShaderProgram = nullptr;
		}

		// Pop a render marker if required
		if (_useRenderMarkers)
		{
			_renderAnnotation->EndEvent();
		}
	}

	// Flush the command queue to minimize render delay. Tests have shown this gives a small but measurable performance
	// boost.
	context->Flush();

	// Forward any pending debug messages to our logging system if required
	if (_enableDebugLogging)
	{
		ProcessPendingDebugMessages();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformLegacyRenderOperation()
{
	// Push a render marker if required
	if (_useRenderMarkers)
	{
		_renderAnnotation->BeginEvent(L"Setup");
	}

	// Initiate any pending data transfer operations within GPU memory
	ID3D11Device1* device = _device.Get();
	ID3D11DeviceContext1* context = _deviceContext.Get();
	for (const auto& entry : _state[_drawIndex].bufferTransferPendingObjects)
	{
		std::visit([device, context](auto* object) { object->CompletePendingDataTransfers(device, context); }, entry);
	}

	// Initiate any pending data transfer operations from CPU to GPU memory
	for (const auto& entry : _state[_drawIndex].bufferUpdatePendingObjects)
	{
		std::visit([device, context](auto* object) { object->CompletePendingDataWrites(device, context); }, entry);
	}

	// Pop a render marker if required
	if (_useRenderMarkers)
	{
		_renderAnnotation->EndEvent();
	}

	// Traverse the render tree and perform our draw operations
	_boundWindowFramebuffers.clear();
	_boundTextureFramebuffers.clear();
	_boundDataArrays.clear();
	_boundTexelArrays.clear();
	Direct3DShaderProgram* currentShaderProgram = nullptr;
	Direct3DFrameBuffer* currentFramebuffer = nullptr;
	size_t boundRenderTargetCount = 0;
	bool isComputeShader = false;
	const std::vector<RenderPassEntry>& renderPassEntries = _state[_drawIndex].renderPasses;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		Direct3DRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// Push a render marker if required
		if (_useRenderMarkers)
		{
			_renderAnnotation->BeginEvent(renderPassNode->DebugName().c_str());
		}

		// Bind the framebuffer for this render pass
		Direct3DFrameBuffer* newFramebuffer = renderPassNode->GetFrameBuffer();
		if (newFramebuffer != currentFramebuffer)
		{
			boundRenderTargetCount = 0;
			if (newFramebuffer != nullptr)
			{
				newFramebuffer->BindFrameBuffer(device, context, _dxgiFactory.Get());
				if (newFramebuffer->IsBoundToWindow())
				{
					_boundWindowFramebuffers.push_back(newFramebuffer);
				}
				else
				{
					_boundTextureFramebuffers.push_back(newFramebuffer);
				}
				boundRenderTargetCount = newFramebuffer->GetRenderTargetViews().size();
			}
			currentFramebuffer = newFramebuffer;
		}

		// Apply fixed state from this render pass node
		renderPassNode->ApplyFixedState(device, context);

		// Render each child node
		const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const Direct3DRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			// If there are no child nodes in this program node, skip it.
			Direct3DProgramNode* programNode = childNodeEntry.node;
			const std::vector<Direct3DStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			if (groupNodes.empty())
			{
				continue;
			}

			// Push a render marker if required
			if (_useRenderMarkers)
			{
				_renderAnnotation->BeginEvent(programNode->DebugName().c_str());
			}

			// Bind the shader program for this program node
			Direct3DShaderProgram* shaderProgram = programNode->GetShaderProgram();
			if (shaderProgram != currentShaderProgram)
			{
				if (currentShaderProgram != nullptr)
				{
					currentShaderProgram->UnbindAllShaderResources(context);
				}
				shaderProgram->BindShaderProgram(context, true);
				currentShaderProgram = shaderProgram;
				isComputeShader = currentShaderProgram->IsComputeShader();
			}

			// Bind any textures or state buffer entries set as defaults at the render pass level
			Direct3DDefaultState* defaultState = childNodeEntry.defaultState;
			const std::vector<ITextureBindingInfo*>* defaultTextureEntries = nullptr;
			const std::vector<ISamplerBindingInfo*>* defaultSamplerEntries = nullptr;
			const std::vector<StateBufferBindingInfo*>* defaultStateBufferEntries = nullptr;
			const std::vector<ResourceArrayBindingInfo*>* defaultResourceBufferEntries = nullptr;
			const std::vector<IStateValueInfo*>* defaultStateValueEntries = nullptr;
			if (defaultState != nullptr)
			{
				defaultTextureEntries = &defaultState->GetTextureEntries();
				defaultSamplerEntries = &defaultState->GetSamplerEntries();
				defaultStateBufferEntries = &defaultState->GetStateBufferEntries();
				defaultResourceBufferEntries = &defaultState->GetResourceBufferEntries();
				defaultStateValueEntries = &defaultState->GetValueEntries();
			}
			bool hasDefaultTextureEntries = (defaultTextureEntries != nullptr) && !defaultTextureEntries->empty();
			bool hasDefaultSamplerEntries = (defaultSamplerEntries != nullptr) && !defaultSamplerEntries->empty();
			bool hasDefaultStateBufferEntries = (defaultStateBufferEntries != nullptr) && !defaultStateBufferEntries->empty();
			bool hasDefaultResourceBufferEntries = (defaultResourceBufferEntries != nullptr) && !defaultResourceBufferEntries->empty();
			bool hasDefaultStateEntries = (defaultStateValueEntries != nullptr) && !defaultStateValueEntries->empty();
			if (hasDefaultTextureEntries)
			{
				BindTextures(*defaultTextureEntries, device, context, shaderProgram);
			}
			if (hasDefaultSamplerEntries)
			{
				BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
			}
			if (hasDefaultStateBufferEntries)
			{
				BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
			}
			if (hasDefaultResourceBufferEntries)
			{
				BindResourceArrays(defaultResourceBufferEntries, nullptr, nullptr, device, context, shaderProgram, true, false, false, boundRenderTargetCount, isComputeShader);
			}

			// Apply constant state values for this program node
			const auto& constantValueEntries = programNode->GetConstantValueEntries();
			shaderProgram->RestoreGlobalConstantBufferBaseline();
			for (IConstantStateValueInfo* constantValue : constantValueEntries)
			{
				constantValue->ApplyValue(shaderProgram);
			}

			// Apply default state values from the program node binding at the render pass level
			if (hasDefaultStateEntries)
			{
				for (const auto& stateValue : *defaultStateValueEntries)
				{
					stateValue->ApplyValue(shaderProgram);
				}
			}

			// Push the current global constant buffer state so we can restore it later
			shaderProgram->PushGlobalConstantBufferState();

			// Render each child node
			bool constantBufferStateDirty = true;
			for (Direct3DStateGroupNode* groupNode : groupNodes)
			{
				// If this state group node is empty, skip it.
				bool isComputeTask = groupNode->HasComputeTask();
				const std::vector<Direct3DRenderableNode*>& renderableNodes = groupNode->GetChildNodes();
				if (!isComputeTask && renderableNodes.empty())
				{
					continue;
				}

				// Push a render marker if required
				if (_useRenderMarkers)
				{
					_renderAnnotation->BeginEvent(groupNode->DebugName().c_str());
				}

				// Apply fixed state associated with this state group node
				groupNode->ApplyFixedState(device, context, currentFramebuffer);

				// Apply state values for this state group node
				const auto& stateValueEntriesForGroupNode = groupNode->GetValueEntries();
				for (const auto& stateValue : stateValueEntriesForGroupNode)
				{
					stateValue->ApplyValue(shaderProgram);
				}
				constantBufferStateDirty = true;

				// Now that we've applied state values, push the current global constant buffer state so we can restore
				// it later.
				shaderProgram->PushGlobalConstantBufferState();

				// Bind any textures or state buffer entries set on this state node
				const auto& textureEntriesForGroupNode = groupNode->GetTextureEntries();
				const auto& samplerEntriesForGroupNode = groupNode->GetSamplerEntries();
				const auto& stateBufferEntriesForGroupNode = groupNode->GetStateBufferEntries();
				const auto& resourceBufferEntriesForGroupNode = groupNode->GetResourceBufferEntries();
				bool hasTextureEntriesForStateGroupNode = !textureEntriesForGroupNode.empty();
				bool hasSamplerEntriesForStateGroupNode = !samplerEntriesForGroupNode.empty();
				bool hasStateBufferEntriesForStateGroupNode = !stateBufferEntriesForGroupNode.empty();
				bool hasResourceBufferEntriesForStateGroupNode = !resourceBufferEntriesForGroupNode.empty();
				if (hasTextureEntriesForStateGroupNode)
				{
					if (hasDefaultTextureEntries)
					{
						BindTextures(*defaultTextureEntries, device, context, shaderProgram);
					}
					BindTextures(textureEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasSamplerEntriesForStateGroupNode)
				{
					if (hasDefaultSamplerEntries)
					{
						BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
					}
					BindSamplers(samplerEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasStateBufferEntriesForStateGroupNode)
				{
					if (hasDefaultStateBufferEntries)
					{
						BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
					}
					BindStateBuffers(stateBufferEntriesForGroupNode, device, context, shaderProgram);
				}
				if (hasResourceBufferEntriesForStateGroupNode)
				{
					BindResourceArrays(defaultResourceBufferEntries, &resourceBufferEntriesForGroupNode, nullptr, device, context, shaderProgram, false, true, false, boundRenderTargetCount, isComputeTask);
				}

				// If this group node is performing a compute task, execute it now.
				if (isComputeTask)
				{
					// Update the global constant buffer if it has uncommitted changes
					if (constantBufferStateDirty)
					{
						shaderProgram->FlushDirtyGlobalConstantBuffers(context);
						constantBufferStateDirty = false;
					}

					// Perform the compute task
					auto threadGroupCounts = groupNode->GetComputeThreadGroupCounts();
					context->Dispatch(threadGroupCounts.X(), threadGroupCounts.Y(), threadGroupCounts.Z());
				}

				// Render each child node
				for (Direct3DRenderableNode* renderableNode : renderableNodes)
				{
					// Push a render marker if required
					if (_useRenderMarkers)
					{
						_renderAnnotation->SetMarker(renderableNode->DebugName().c_str());
					}

					// Apply state values for this renderable node
					const auto& stateValueEntriesForRenderableNode = renderableNode->GetValueEntries();
					bool hasStateValues = !stateValueEntriesForRenderableNode.empty();
					if (hasStateValues)
					{
						for (const auto& stateValue : stateValueEntriesForRenderableNode)
						{
							stateValue->ApplyValue(shaderProgram);
						}
						constantBufferStateDirty = true;
					}

					// Update the global constant buffer if it has uncommitted changes
					if (constantBufferStateDirty)
					{
						shaderProgram->FlushDirtyGlobalConstantBuffers(context);
						constantBufferStateDirty = false;
					}

					// Bind any textures or state buffer entries set on this renderable node
					const auto& textureEntriesForRenderableNode = renderableNode->GetTextureEntries();
					const auto& samplerEntriesForRenderableNode = renderableNode->GetSamplerEntries();
					const auto& stateBufferEntriesForRenderableNode = renderableNode->GetStateBufferEntries();
					const auto& resourceBufferEntriesForRenderableNode = renderableNode->GetResourceBufferEntries();
					bool hasTextureEntries = !textureEntriesForRenderableNode.empty();
					bool hasSamplerEntries = !samplerEntriesForRenderableNode.empty();
					bool hasStateBufferEntries = !stateBufferEntriesForRenderableNode.empty();
					bool hasResourceBufferEntries = !resourceBufferEntriesForRenderableNode.empty();
					if (hasTextureEntries)
					{
						BindTextures(textureEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasSamplerEntries)
					{
						BindSamplers(samplerEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasStateBufferEntries)
					{
						BindStateBuffers(stateBufferEntriesForRenderableNode, device, context, shaderProgram);
					}
					if (hasResourceBufferEntries)
					{
						BindResourceArrays(defaultResourceBufferEntries, (hasResourceBufferEntriesForStateGroupNode ? &resourceBufferEntriesForGroupNode : nullptr), &resourceBufferEntriesForRenderableNode, device, context, shaderProgram, false, false, true, boundRenderTargetCount, false);
					}

					// If the renderable node has been setup to use indirect multi-draw with a draw count retrieved from
					// GPU memory, retrieve the value now. This will be inefficient, as it involves a CPU round trip and
					// will therefore cause a pipeline stall. We accept this here to allow Direct3D 11 to emulate
					// indirect multi-draw, which it doesn't natively support.
					if (renderableNode->UsesIndirectMultiDrawWithVariableCount())
					{
						// Obtain the settings on which buffer to read the draw counter from, and the location of the
						// value within the buffer.
						Direct3DDataArray* drawCountBuffer;
						bool drawCountFromCounter;
						size_t drawCountBufferOffsetInBytes;
						renderableNode->GetIndirectMultiDrawCountSourceBufferInfo(drawCountBuffer, drawCountFromCounter, drawCountBufferOffsetInBytes);

						// Read the draw counter value from the buffer
						UINT drawCount;
						if (drawCountFromCounter)
						{
							drawCount = drawCountBuffer->GetCurrentCounterValue(device, context);
						}
						else
						{
							drawCountBuffer->GetCurrentBufferData(device, context, drawCountBufferOffsetInBytes, &drawCount, sizeof(drawCount));
						}

						// Update the current draw count value in the renderable node
						renderableNode->SetCurrentIndirectDrawCount(drawCount);
					}

					// Draw this renderable object
					renderableNode->Draw(shaderProgram, context);

					// If we changed state values for this renderable, restore it to the pushed state baseline.
					if (hasStateValues)
					{
						shaderProgram->RestoreGlobalConstantBufferBaseline();
						constantBufferStateDirty = true;
					}

					// If we bound any textures or state buffer entries, and the state group node provided some initial
					// bindings of its own, restore the bindings that were set on the state group node now.
					if (hasTextureEntries)
					{
						if (hasDefaultTextureEntries)
						{
							BindTextures(*defaultTextureEntries, device, context, shaderProgram);
						}
						if (hasTextureEntriesForStateGroupNode)
						{
							BindTextures(textureEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasSamplerEntries)
					{
						if (hasDefaultSamplerEntries)
						{
							BindSamplers(*defaultSamplerEntries, device, context, shaderProgram);
						}
						if (hasSamplerEntriesForStateGroupNode)
						{
							BindSamplers(samplerEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasStateBufferEntries)
					{
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(*defaultStateBufferEntries, device, context, shaderProgram);
						}
						if (hasStateBufferEntriesForStateGroupNode)
						{
							BindStateBuffers(stateBufferEntriesForGroupNode, device, context, shaderProgram);
						}
					}
					if (hasResourceBufferEntries)
					{
						if (hasResourceBufferEntriesForStateGroupNode)
						{
							BindResourceArrays(defaultResourceBufferEntries, &resourceBufferEntriesForGroupNode, nullptr, device, context, shaderProgram, false, false, false, boundRenderTargetCount, false);
						}
						else if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(defaultResourceBufferEntries, nullptr, nullptr, device, context, shaderProgram, false, false, false, boundRenderTargetCount, false);
						}
					}
				}

				// Pop the global constant buffer state
				shaderProgram->PopGlobalConstantBufferState();

				// Pop a render marker if required
				if (_useRenderMarkers)
				{
					_renderAnnotation->EndEvent();
				}
			}

			// Pop the global constant buffer state
			shaderProgram->PopGlobalConstantBufferState();

			// Pop a render marker if required
			if (_useRenderMarkers)
			{
				_renderAnnotation->EndEvent();
			}
		}

		// Perform any unbind operations for the render pass node. Currently this is limited to multisample framebuffer
		// resolution.
		renderPassNode->PerformUnbindOperations(context);

		// Unbind any current shader resources, to prevent warnings about active shader resources being bound to
		// framebuffer output targets.
		if (currentShaderProgram != nullptr)
		{
			currentShaderProgram->UnbindAllShaderResources(context);
			currentShaderProgram = nullptr;
		}

		// Pop a render marker if required
		if (_useRenderMarkers)
		{
			_renderAnnotation->EndEvent();
		}
	}

	// Flush the command queue to minimize render delay. Tests have shown this gives a small but measurable performance
	// boost.
	context->Flush();

	// Forward any pending debug messages to our logging system if required
	if (_enableDebugLogging)
	{
		ProcessPendingDebugMessages();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformSwapOperation()
{
	// Capture the output of any framebuffers as required. We need to do this before the present in Direct3D 11 in order
	// to support the flip presentation modes, as we can't rely on the contents of the buffers after they're swapped,
	// and we can't queue a transfer operation like we can under Direct3D 12.
	_capturedFramebufferOutputsInCurrentFrame.clear();
	_capturedDataArrayOutputsInCurrentFrame.clear();
	_capturedTexelArrayOutputsInCurrentFrame.clear();
	ID3D11Device1* device = _device.Get();
	ID3D11DeviceContext1* deviceContext = _deviceContext.Get();
	for (Direct3DFrameBuffer* frameBuffer : _boundWindowFramebuffers)
	{
		frameBuffer->CaptureFrameBufferOutput(device, deviceContext);
	}
	for (Direct3DFrameBuffer* frameBuffer : _boundTextureFramebuffers)
	{
		frameBuffer->CaptureFrameBufferOutput(device, deviceContext);
	}
	for (Direct3DDataArray* resourceBuffer : _boundDataArrays)
	{
		resourceBuffer->CaptureDataBufferOutput(device, deviceContext);
	}
	for (Direct3DTexelArray* resourceBuffer : _boundTexelArrays)
	{
		resourceBuffer->CaptureDataBufferOutput(device, deviceContext);
	}

	// Swap any framebuffers to the screen as required
	for (Direct3DFrameBuffer* frameBuffer : _boundWindowFramebuffers)
	{
		frameBuffer->PresentToWindow();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformDeleteLastDrawResourcesOperation()
{
	// Delete all pending objects to be deleted
	for (const auto& entry : _state[_drawIndex].deletePendingObjects)
	{
		std::visit([](auto* object) { delete object; }, entry);
	}
	_state[_drawIndex].deletePendingObjects.clear();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformDeleteNextDrawResourcesOperation()
{
	// Since we're advancing this delete step, strip the objects being deleted out of any pending update operations
	// which would normally be run first.
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	std::unordered_set<void*> deleteSet;
	for (const auto& entry : _state[_buildIndex].deletePendingObjects)
	{
		deleteSet.insert(std::visit([](auto* object) { return static_cast<void*>(object); }, entry));
	}
	_state[_buildIndex].migrateStatePendingObjects.erase(std::remove_if(_state[_buildIndex].migrateStatePendingObjects.begin(), _state[_buildIndex].migrateStatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].migrateStatePendingObjects.end());
	_state[_buildIndex].bufferUpdatePendingObjects.erase(std::remove_if(_state[_buildIndex].bufferUpdatePendingObjects.begin(), _state[_buildIndex].bufferUpdatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferUpdatePendingObjects.end());
	_state[_buildIndex].bufferTransferPendingObjects.erase(std::remove_if(_state[_buildIndex].bufferTransferPendingObjects.begin(), _state[_buildIndex].bufferTransferPendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferTransferPendingObjects.end());
	_capturedFramebufferOutputsInCurrentFrame.erase(std::remove_if(_capturedFramebufferOutputsInCurrentFrame.begin(), _capturedFramebufferOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedFramebufferOutputsInCurrentFrame.end());
	_capturedDataArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedDataArrayOutputsInCurrentFrame.begin(), _capturedDataArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedDataArrayOutputsInCurrentFrame.end());
	_capturedTexelArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedTexelArrayOutputsInCurrentFrame.begin(), _capturedTexelArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedTexelArrayOutputsInCurrentFrame.end());

	// Delete all pending objects to be deleted
	for (const auto& entry : _state[_buildIndex].deletePendingObjects)
	{
		std::visit([](auto* object) { delete object; }, entry);
	}
	_state[_buildIndex].deletePendingObjects.clear();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindTexture(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (ISamplerBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindSampler(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindStateBuffer(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>* defaultBindingEntries, const std::vector<ResourceArrayBindingInfo*>* groupNodeBindingEntries, const std::vector<ResourceArrayBindingInfo*>* renderableBindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool resetCountersForDefaultBindingEntries, bool resetCountersForGroupBindingEntries, bool resetCountersForRenderableBindingEntries, size_t boundRenderTargetCount, bool computeShaderBinding)
{
	// Build our arrays of resource array views and reset values to apply
	_resourceBufferViews.clear();
	_resourceBufferResetValues.clear();
	UINT lowestBindPoint = 0xFFFFFFFF;
	if (defaultBindingEntries != nullptr)
	{
		UpdateResourceBufferEntries(*defaultBindingEntries, device, context, program, resetCountersForDefaultBindingEntries, _resourceBufferViews, _resourceBufferResetValues, lowestBindPoint);
	}
	if (groupNodeBindingEntries != nullptr)
	{
		UpdateResourceBufferEntries(*groupNodeBindingEntries, device, context, program, resetCountersForGroupBindingEntries, _resourceBufferViews, _resourceBufferResetValues, lowestBindPoint);
	}
	if (renderableBindingEntries != nullptr)
	{
		UpdateResourceBufferEntries(*renderableBindingEntries, device, context, program, resetCountersForRenderableBindingEntries, _resourceBufferViews, _resourceBufferResetValues, lowestBindPoint);
	}

	// If no read/write views were found for our shader resource arrays, abort any further processing.
	if (lowestBindPoint == 0xFFFFFFFF)
	{
		return;
	}

	// Ensure that sensible values were encountered for our lowest bind point
	if (lowestBindPoint < boundRenderTargetCount)
	{
		_log->Error("Attempted to bind resource arrays starting at binding point {0} when there are {1} render targets currently bound.", lowestBindPoint, boundRenderTargetCount);
		return;
	}

	// Update the bound UAV views for our resource arrays
	if (computeShaderBinding)
	{
		context->CSSetUnorderedAccessViews(lowestBindPoint, (UINT)(_resourceBufferViews.size() - lowestBindPoint), (_resourceBufferViews.data() + lowestBindPoint), (_resourceBufferResetValues.data() + lowestBindPoint));
	}
	else
	{
		context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, lowestBindPoint, (UINT)(_resourceBufferViews.size() - lowestBindPoint), (_resourceBufferViews.data() + lowestBindPoint), (_resourceBufferResetValues.data() + lowestBindPoint));
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UpdateResourceBufferEntries(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool performReset, std::vector<ID3D11UnorderedAccessView*>& resourceBufferViews, std::vector<UINT>& resetValues, UINT& lowestBindPoint)
{
	for (auto* entry : bindingEntries)
	{
		entry->BindResourceArray(device, context, program, performReset, resourceBufferViews, resetValues, lowestBindPoint);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindTexture(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (ISamplerBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindSampler(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindStateBuffer(device, context, program);
	}
}

//----------------------------------------------------------------------------------------
// Resource methods
//----------------------------------------------------------------------------------------
ID3D11Device1* Direct3DRenderer::GetDevice() const
{
	return _device.Get();
}

//----------------------------------------------------------------------------------------
// Settings methods
//----------------------------------------------------------------------------------------
bool Direct3DRenderer::DebugLoggingEnabled() const
{
	return _enableDebugLogging;
}

//----------------------------------------------------------------------------------------
bool Direct3DRenderer::UseDeferredBufferCreation() const
{
	// This setting controls whether the creation of buffers (currently vertex and index buffers) is deferred to the
	// render thread, or whether it is performed in parallel with rendering on the calling application thread when the
	// buffer is allocated. Deferring buffer creation leaves the calling application thread unblocked, giving it the
	// appearance of very quick buffer creation, however this is only an illusion. In reality, all the work has been
	// deferred for the render thread to pick up later, and it will have to perform the same tasks but in a blocking
	// single-threaded manner, holding up rendering tasks. Since the application is able to add new buffers much more
	// quickly than the renderer can process them with deferred buffer creation, it's possible to create extremely long
	// delays between frames if a large number of resources have been allocated. It is strongly recommended not to use
	// deferred buffer creation, but instead to push the overhead of buffer creation to the caller. This will result in
	// slower buffer allocation times from the perspective of the caller, but it will perform allocations in a
	// multithreaded manner, and the framerate of drawing should be fairly stable and consistent during the process.
	//##DEBUG##
	// return true;
	return false;
}

//----------------------------------------------------------------------------------------
bool Direct3DRenderer::UseLegacyRenderingMethod() const
{
	// This setting controls which approach is used to managing global constant buffer state changes during rendering.
	// Changing constant buffer state during rendering is problematic, as the hardware prefers resources to be fully
	// updated ahead of time, followed by a series of draw calls. Where data changes are mixed in with draw calls, there
	// are performance penalties. The problem is much more serious when the same resource is used by multiple objects,
	// and each object needs to be rendered with the values which were current at the time they were drawn. This is
	// exactly what happens with global constant shader buffers, where there are often numerous basic shader settings
	// that need to be changed between draw calls, with potentially a data change to each constant buffer required
	// between each draw call. Fortunately drivers manage a lot of complexity behind the Direct3D 11 API for us, and do
	// a lot of work with constant buffers to automatically duplicate them into successive "pages", so that the drawing
	// process is more efficient. See the following article from NVidia for some more information on this topic:
	// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
	// Our legacy rendering method here relies on letting the hardware split out repeated edits to the constant buffers
	// into multiple pages for us. The new rendering method instead parses the scene before the draw operation, and
	// builds a paged global constant buffer for the entire scene, then binds the appropriate page to each object when
	// they are drawn. This eliminates all constant buffer modifications after draw calls commence, and has been shown
	// to achieve slightly better performance than the driver manages on its own. That said, the performance gap isn't
	// huge, and the new rendering technique relies on Direct3D 11.1 hardware features. The old rendering method is
	// therefore retained here for comparison purposes, and to allow for supporting Direct3D 11.0 hardware.
	//##DEBUG##
	// return true;
	return false;
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
bool Direct3DRenderer::IsFeaturePresent(DXGI_FEATURE feature) const
{
	// Try and get the IDXGIFactory5 interface. If that's not available, the feature definitely isn't either.
	IDXGIFactory5* _dxgiFactory5 = nullptr;
	HRESULT queryFactory5Return = _dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(&_dxgiFactory5));
	if (FAILED(queryFactory5Return))
	{
		return false;
	}

	// Check if the feature is supported
	BOOL featureSupported = FALSE;
	HRESULT checkFeatureSupportReturn = _dxgiFactory5->CheckFeatureSupport(feature, &featureSupported, sizeof(featureSupported));
	if (FAILED(checkFeatureSupportReturn))
	{
		_log->Error("CheckFeatureSupport failed with error code {0}", checkFeatureSupportReturn);
		return false;
	}
	return (featureSupported == TRUE);
}

} // namespace cobalt::graphics
