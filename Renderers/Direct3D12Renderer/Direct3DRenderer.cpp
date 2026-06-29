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
#include <type_traits>
#include <utility>
#define USE_PIX
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 6011)
#pragma warning(disable : 6101)
#endif
#include <pix3.h>
WARNINGS_POP
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DRenderer::Direct3DRenderer(cobalt::logging::ILogger::unique_ptr log, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter, const Marshal::In<std::set<IGraphicsDevice::Feature>>& enabledFeatures, const Marshal::In<std::set<Options>>& enabledOptions)
: _buildIndex(0), _drawIndex(1), _dxgiFactory(std::move(std::move(dxgiFactory))), _adapter(std::move(std::move(adapter))), _enabledOptions(enabledOptions)
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderer::Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags)
{
	_enableDebugLogging = (_enabledOptions.find(Options::EnableDebugLogging) != _enabledOptions.end());
	_useRenderMarkers = (_enabledOptions.find(Options::EnableRenderMarkers) != _enabledOptions.end());

	// Enable the debug layer if required
	UINT dxgiCreateFlags = 0;
	if (_enableDebugLogging)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		HRESULT getDebugInterfaceReturn = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		if (FAILED(getDebugInterfaceReturn))
		{
			_log->Error("D3D12GetDebugInterface failed with error code {0}", getDebugInterfaceReturn);
			return false;
		}
		debugController->EnableDebugLayer();
		dxgiCreateFlags = DXGI_CREATE_FACTORY_DEBUG;
	}

	// Retrieve the target adapter to create the device for
	IDXGIAdapter2* targetAdapter = _adapter.Get();
	bool hasTargetAdapter = (targetAdapter != nullptr);

	// If a target adapter has been supplied, but we've also selected some DXGI create flags to enable, we need to create a
	// new DXGI factory and select the target adapter again.
	if (hasTargetAdapter && (dxgiCreateFlags != 0))
	{
		// Retrieve the device ID of the selected target adapter
		DXGI_ADAPTER_DESC2 existingAdapterDescription;
		HRESULT getTargetAdapterDescriptionReturn = targetAdapter->GetDesc2(&existingAdapterDescription);
		if (FAILED(getTargetAdapterDescriptionReturn))
		{
			_log->Error("targetAdapter->GetDesc2() failed with error code {0}", getTargetAdapterDescriptionReturn);
			return false;
		}
		UINT targetAdapterDeviceId = existingAdapterDescription.DeviceId;

		// Create a new DXGI factory
		HRESULT createDxgiFactory2Return = CreateDXGIFactory2(dxgiCreateFlags, IID_PPV_ARGS(&_dxgiFactory));
		if (FAILED(createDxgiFactory2Return))
		{
			_log->Error("CreateDXGIFactory2 failed with error code {0}", createDxgiFactory2Return);
			return false;
		}

		// Enumerate all available adapters, and record information on each one.
		bool foundTargetAdapter = false;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapterV1;
		uint32_t adapterNo = 0;
		while (!foundTargetAdapter && !FAILED(_dxgiFactory->EnumAdapters1(adapterNo, &adapterV1)))
		{
			// Cast the adapter to the required type
			Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter;
			HRESULT castAdapterReturn = adapterV1.As(&adapter);
			if (FAILED(castAdapterReturn))
			{
				_log->Warning("Failed to cast adapter to IDXGIAdapter3 with error code {0}", castAdapterReturn);
				++adapterNo;
				continue;
			}

			// Retrieve information on the adapter
			DXGI_ADAPTER_DESC2 adapterDescription;
			HRESULT getAdapterDescriptionReturn = adapter->GetDesc2(&adapterDescription);
			if (FAILED(getAdapterDescriptionReturn))
			{
				_log->Warning("Failed to retrieve adapter description with error code {0}", getAdapterDescriptionReturn);
				++adapterNo;
				continue;
			}

			// If we've located the target adapter, select it.
			if (adapterDescription.DeviceId == targetAdapterDeviceId)
			{
				_adapter = std::move(adapter);
				foundTargetAdapter = true;
				continue;
			}

			// Advance to the next adapter entry
			++adapterNo;
		}

		// If we failed to locate the target adapter, abort any further processing.
		if (!foundTargetAdapter)
		{
			_log->Error("Failed to locate target adapter when re-opening with CreateDXGIFactory2.");
			return false;
		}
	}

	// If a target adapter hasn't been supplied, create a DXGI factory.
	if (!hasTargetAdapter)
	{
		if (dxgiCreateFlags != 0)
		{
			HRESULT createDxgiFactory2Return = CreateDXGIFactory2(dxgiCreateFlags, IID_PPV_ARGS(&_dxgiFactory));
			if (FAILED(createDxgiFactory2Return))
			{
				_log->Error("CreateDXGIFactory2 failed with error code {0}", createDxgiFactory2Return);
				return false;
			}
		}
		else
		{
			HRESULT createDxgiFactory1Return = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
			if (FAILED(createDxgiFactory1Return))
			{
				_log->Error("CreateDXGIFactory1 failed with error code {0}", createDxgiFactory1Return);
				return false;
			}
		}
	}

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

	// Attempt to initialize Direct3D, requesting the 11.0 feature level as a minimum.
	D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	HRESULT d3d12CreateDeviceReturn = D3D12CreateDevice(targetAdapter, minimumFeatureLevel, IID_PPV_ARGS(&_device));
	if (FAILED(d3d12CreateDeviceReturn))
	{
		_log->Error("D3D12CreateDevice failed with error code {0}", d3d12CreateDeviceReturn);
		return false;
	}

	// If a target adapter hasn't been supplied, retrieve the adapter associated with the created device.
	if (!hasTargetAdapter)
	{
		HRESULT enumAdapterByLuidReturn = _dxgiFactory->EnumAdapterByLuid(_device->GetAdapterLuid(), IID_PPV_ARGS(&_adapter));
		if (FAILED(enumAdapterByLuidReturn))
		{
			_log->Error("EnumAdapterByLuid failed with error code {0}", enumAdapterByLuidReturn);
			return false;
		}
	}

	// Route debug messages through our logging system and filter out noise which isn't relevant for our usage
	if (_enableDebugLogging)
	{
		HRESULT queryInfoQueueReturn = _device.As(&_debugInfoQueue);
		if (FAILED(queryInfoQueueReturn))
		{
			_log->Warning("Failed to retrieve ID3D12InfoQueue with error code {0}. API debug logging will not be redirected to internal log system.", queryInfoQueueReturn);
		}
		else
		{
			// Enable break on severe errors for debug builds
#ifdef _DEBUG
			_debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
#endif

			// Filter out messages we don't consider useful
			auto messageFilterList = {
			  D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			  D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
			  D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_DEPTHSTENCILVIEW_NOT_SET,
			  D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_VERTEX_BUFFER_NOT_SET,
			  D3D12_MESSAGE_ID_CREATE_CONSTANT_BUFFER_VIEW_INVALID_RESOURCE,
			  // 2022-07-28 - Added this exclusion to work around a validation layer bug on Windows 11 as described at https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			  D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
			};
			D3D12_INFO_QUEUE_FILTER messageFilter = {};
			messageFilter.DenyList.NumIDs = (UINT)messageFilterList.size();
			messageFilter.DenyList.pIDList = const_cast<D3D12_MESSAGE_ID*>(messageFilterList.begin());
			HRESULT addStorageFilterEntriesReturn = _debugInfoQueue->AddStorageFilterEntries(&messageFilter);
			if (FAILED(addStorageFilterEntriesReturn))
			{
				_log->Warning("Failed to add debug message filter with error code {0}", addStorageFilterEntriesReturn);
			}

			// Route Direct3D 12 debug output through our logging system if the newer callback API is available
			HRESULT queryInfoQueue1Return = _device.As(&_debugInfoQueue1);
			if (SUCCEEDED(queryInfoQueue1Return))
			{
				HRESULT registerMessageCallbackReturn = _debugInfoQueue1->RegisterMessageCallback(DebugMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &_debugMessageCallbackCookie);
				if (FAILED(registerMessageCallbackReturn))
				{
					_log->Warning("ID3D12InfoQueue1::RegisterMessageCallback failed with error code {0}", registerMessageCallbackReturn);
					_debugInfoQueue1.Reset();
				}
				else
				{
					// Release the older ID3D12InfoQueue interface, since we'll now be handling logging via our
					// callback.
					_debugInfoQueue.Reset();
				}
			}
		}
	}

	// Create our command queues
	D3D12_COMMAND_QUEUE_DESC drawCommandQueueDescription = {};
	drawCommandQueueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	drawCommandQueueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT createDrawCommandQueueReturn = _device->CreateCommandQueue(&drawCommandQueueDescription, IID_PPV_ARGS(&_drawCommandQueue));
	if (FAILED(createDrawCommandQueueReturn))
	{
		_log->Error("CreateCommandQueue failed for draw queue with error code {0}", createDrawCommandQueueReturn);
		return false;
	}
	D3D12_COMMAND_QUEUE_DESC buildCommandQueueDescription = {};
	buildCommandQueueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	buildCommandQueueDescription.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	HRESULT createBuildCommandQueueReturn = _device->CreateCommandQueue(&buildCommandQueueDescription, IID_PPV_ARGS(&_buildCommandQueue));
	if (FAILED(createBuildCommandQueueReturn))
	{
		_log->Error("CreateCommandQueue failed for build queue with error code {0}", createBuildCommandQueueReturn);
		return false;
	}

	// Create our draw command allocator and command list
	HRESULT createDrawCommandAllocatorReturn = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_drawCommandAllocator));
	if (FAILED(createDrawCommandAllocatorReturn))
	{
		_log->Error("CreateCommandAllocator failed for draw commands with error code {0}", createDrawCommandAllocatorReturn);
		return false;
	}
	HRESULT createDrawCommandListReturn = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _drawCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&_drawCommandList));
	if (FAILED(createDrawCommandListReturn))
	{
		_log->Error("CreateCommandList failed for draw command list with error code {0}", createDrawCommandListReturn);
		return false;
	}
	_drawCommandList->Close();

	// Create a fence object for our build commands
	HRESULT createBuildFenceReturn = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_buildFence));
	if (FAILED(createBuildFenceReturn))
	{
		_log->Error("Failed to create build fence with error code {0}", createBuildFenceReturn);
		return false;
	}

	// Create an event to raise when the fence is reached
	_buildFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_buildFenceEvent == nullptr)
	{
		DWORD createBuildFenceEventLastError = GetLastError();
		_log->Error("Failed to create build fence event with error code {0}", createBuildFenceEventLastError);
		return false;
	}

	// Configure the fence to raise our event when it is reached
	HRESULT setBuildFenceEventOnCompletionReturn = _buildFence->SetEventOnCompletion(1, _buildFenceEvent);
	if (FAILED(setBuildFenceEventOnCompletionReturn))
	{
		_log->Error("Failed to raise event on build fence completion with error code {0}", setBuildFenceEventOnCompletionReturn);
		return false;
	}

	// Create a fence object for our draw commands
	HRESULT createDrawFenceReturn = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_drawFence));
	if (FAILED(createDrawFenceReturn))
	{
		_log->Error("Failed to create draw fence with error code {0}", createDrawFenceReturn);
		return false;
	}

	// Create an event to raise when the fence is reached
	_drawFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_drawFenceEvent == nullptr)
	{
		DWORD createDrawFenceEventLastError = GetLastError();
		_log->Error("Failed to create draw fence event with error code {0}", createDrawFenceEventLastError);
		return false;
	}

	// Configure the fence to raise our event when it is reached
	HRESULT setDrawFenceEventOnCompletionReturn = _drawFence->SetEventOnCompletion(1, _drawFenceEvent);
	if (FAILED(setDrawFenceEventOnCompletionReturn))
	{
		_log->Error("Failed to raise event on draw fence completion with error code {0}", setDrawFenceEventOnCompletionReturn);
		return false;
	}

	// Allocate our heap manager
	_heapManager = std::make_unique<Direct3DHeapManager>(_log.get(), _device.Get());

	// Create our memory manager
	D3D12MA::ALLOCATOR_DESC allocatorDescription{};
	allocatorDescription.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
	allocatorDescription.pDevice = _device.Get();
	allocatorDescription.PreferredBlockSize = 0;
	allocatorDescription.pAllocationCallbacks = nullptr;
	allocatorDescription.pAdapter = _adapter.Get();
	HRESULT createAllocatorReturn = D3D12MA::CreateAllocator(&allocatorDescription, &_memoryManager);
	if (FAILED(createAllocatorReturn))
	{
		_log->Error("Failed to create memory allocator with error code {0}", createAllocatorReturn);
		return false;
	}

	// Create our residency manager
	_residencyManager = std::make_unique<D3DX12Residency::ResidencyManager>();
	HRESULT initializeResidencyManagerReturn = _residencyManager->Initialize(_device.Get(), 0, _adapter.Get(), 1 * 5);
	if (FAILED(initializeResidencyManagerReturn))
	{
		_log->Error("Failed to initialize residency manager with error code {0}", initializeResidencyManagerReturn);
		return false;
	}

	// Allocate our build command list pool
	_buildCommandListPool = std::make_unique<Direct3DCommandListPool>(_log.get(), _device.Get(), _buildCommandQueue.Get(), _residencyManager.get());

	// Create a residency set for our draw process
	_drawResidencySet = _residencyManager->CreateResidencySet();

	// Create an event to raise when we want to shut down the video memory budget change worker thread
	_videoMemoryBudgetShutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (_videoMemoryBudgetShutdownEvent == nullptr)
	{
		DWORD createVideoMemoryBudgetShutdownEventLastError = GetLastError();
		_log->Error("Failed to create video memory budget thread shutdown event with error code {0}", createVideoMemoryBudgetShutdownEventLastError);
		return false;
	}

	// Start the video memory budget change worker thread
	_videoMemoryBudgetChangeWorkerThreadActive = true;
	std::thread videoMemoryBudgetChangeWorkerThread(std::bind(std::mem_fn(&Direct3DRenderer::VideoMemoryBudgetChangeWorkerThread), this));
	videoMemoryBudgetChangeWorkerThread.detach();

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
	std::thread renderWorkerThread(std::bind(std::mem_fn(&Direct3DRenderer::RenderThread), this));
	renderWorkerThread.detach();
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

	// Terminate the video memory budget change worker thread
	if (_videoMemoryBudgetChangeWorkerThreadActive)
	{
		_videoMemoryBudgetChangeWorkerThreadActive = false;
		if (SetEvent(_videoMemoryBudgetShutdownEvent) == FALSE)
		{
			DWORD setVideoMemoryBudgetShutdownEventLastError = GetLastError();
			_log->Error("SetEvent failed on _videoMemoryBudgetShutdownEvent with error code {0}", setVideoMemoryBudgetShutdownEventLastError);
		}
		else
		{
			_notifyVideoMemoryBudgetChangeWorkerStopped.wait(lock);
		}
	}
	lock.unlock();

	// Delete any objects which are pending deletion
	PerformDeleteLastDrawResourcesOperation();
	PerformDeleteNextDrawResourcesOperation();

	// If any temporary transfer buffers were allocated by either the last draw or build phase, destroy them now.
	_state[_buildIndex].transferBufferAllocations.clear();
	_state[_drawIndex].transferBufferAllocations.clear();

	// Destroy the draw residency set
	_residencyManager->DestroyResidencySet(_drawResidencySet);

	// Delete the memory manager
	if (_memoryManager != nullptr)
	{
		_memoryManager->Release();
		_memoryManager = nullptr;
	}

	// Release our various held objects before releasing the Direct3D device
	_heapManager.reset();
	_buildCommandListPool.reset();
	_residencyManager.reset();
	_buildFence.Reset();
	_drawCommandQueue.Reset();
	_buildCommandQueue.Reset();
	_drawCommandAllocator.Reset();
	_drawFence.Reset();
	_drawCommandList.Reset();
	_dxgiFactory.Reset();
	_adapter.Reset();

	// Release our device object
	_device.Reset();

	// Process any remaining debug messages, and release the info queue.
	if (_enableDebugLogging && (_debugInfoQueue.Get() != nullptr))
	{
		ProcessPendingDebugMessages();
		_debugInfoQueue.Reset();
		_dxgiInfoQueue.Reset();
	}

	// Unregister any debug message callback now that the device has been released
	if (_debugInfoQueue1 != nullptr)
	{
		HRESULT unregisterMessageCallbackReturn = _debugInfoQueue1->UnregisterMessageCallback(_debugMessageCallbackCookie);
		if (FAILED(unregisterMessageCallbackReturn))
		{
			_log->Warning("UnregisterMessageCallback failed with error code {0}", unregisterMessageCallbackReturn);
		}
		_debugInfoQueue1.Reset();
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

	// Dispose of our event handles
	CloseHandle(_buildFenceEvent);
	CloseHandle(_drawFenceEvent);
	CloseHandle(_videoMemoryBudgetShutdownEvent);

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
const char* Direct3DRenderer::DebugMessageCategoryToString(D3D12_MESSAGE_CATEGORY category)
{
	switch (category)
	{
	case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:
		return "Application";
	case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:
		return "Miscellaneous";
	case D3D12_MESSAGE_CATEGORY_INITIALIZATION:
		return "Initialization";
	case D3D12_MESSAGE_CATEGORY_CLEANUP:
		return "Cleanup";
	case D3D12_MESSAGE_CATEGORY_COMPILATION:
		return "Compilation";
	case D3D12_MESSAGE_CATEGORY_STATE_CREATION:
		return "StateCreation";
	case D3D12_MESSAGE_CATEGORY_STATE_SETTING:
		return "StateSetting";
	case D3D12_MESSAGE_CATEGORY_STATE_GETTING:
		return "StateGetting";
	case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:
		return "ResourceManipulation";
	case D3D12_MESSAGE_CATEGORY_EXECUTION:
		return "Execution";
	case D3D12_MESSAGE_CATEGORY_SHADER:
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
void Direct3DRenderer::DebugMessageCallbackInternal(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID messageId, LPCSTR description) const
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
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
	case D3D12_MESSAGE_SEVERITY_ERROR:
		_log->Error("Direct3D 12 Error [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		_log->Warning("Direct3D 12 Warning [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D12_MESSAGE_SEVERITY_INFO:
		_log->Info("Direct3D 12 Info [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		_log->Debug("Direct3D 12 Debug [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription);
		break;
	default:
		_log->Debug("Direct3D 12 ({3}) [{0}] {1}: {2}", categoryString, (uint32_t)messageId, messageDescription, (uint32_t)severity);
		break;
	}
}

//----------------------------------------------------------------------------------------
void CALLBACK Direct3DRenderer::DebugMessageCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID messageId, LPCSTR description, void* context)
{
	auto* renderer = reinterpret_cast<Direct3DRenderer*>(context);
	renderer->DebugMessageCallbackInternal(category, severity, messageId, description);
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
				_log->Error("ID3D12InfoQueue::GetMessage failed with error code {0}", getMessageReturn);
				continue;
			}

			// Resize our scratch buffer to be large enough to hold the message
			_debugMessageBuffer.resize(messageLength);
			auto* message = reinterpret_cast<D3D12_MESSAGE*>(_debugMessageBuffer.data());

			// Retrieve the next message
			getMessageReturn = _debugInfoQueue->GetMessage(i, message, &messageLength);
			if (FAILED(getMessageReturn))
			{
				_log->Error("ID3D12InfoQueue::GetMessage failed with error code {0}", getMessageReturn);
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
void Direct3DRenderer::VideoMemoryBudgetChangeWorkerThread()
{
	// Create an event to raise when the video memory budget changes
	HANDLE videoMemoryBudgetChangeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (videoMemoryBudgetChangeEvent == nullptr)
	{
		DWORD createVideoMemoryBudgetChangeEventLastError = GetLastError();
		_log->Error("Failed to create video memory budget change event with error code {0}", createVideoMemoryBudgetChangeEventLastError);
		std::unique_lock<std::mutex> lock(_renderThreadMutex);
		_notifyVideoMemoryBudgetChangeWorkerStopped.notify_all();
		return;
	}

	// Register a callback on video memory budget changes
	DWORD videoMemoryBudgetChangeEventRegistrationCookie;
	HRESULT registerVideoMemoryBudgetChangeNotificationEventReturn = _adapter->RegisterVideoMemoryBudgetChangeNotificationEvent(videoMemoryBudgetChangeEvent, &videoMemoryBudgetChangeEventRegistrationCookie);
	if (FAILED(registerVideoMemoryBudgetChangeNotificationEventReturn))
	{
		_log->Error("RegisterVideoMemoryBudgetChangeNotificationEvent failed with error code {0}", registerVideoMemoryBudgetChangeNotificationEventReturn);
	}

	// Process change notifications until we receive a shutdown request
	HANDLE eventHandles[] = {videoMemoryBudgetChangeEvent, _videoMemoryBudgetShutdownEvent};
	bool shutdownRequested = false;
	while (!shutdownRequested)
	{
		// Update the video memory budget
		DXGI_QUERY_VIDEO_MEMORY_INFO queryVideoMemoryInfo;
		HRESULT queryVideoMemoryInfoReturn = _adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &queryVideoMemoryInfo);
		if (FAILED(queryVideoMemoryInfoReturn))
		{
			_log->Error("QueryVideoMemoryInfo failed with error code {0}", queryVideoMemoryInfoReturn);
		}
		else
		{
			_videoMemoryBudget = (size_t)queryVideoMemoryInfo.Budget;
			double videoMemoryBudgetInGB = (double)_videoMemoryBudget / (1024.0 * 1024.0 * 1024.0);
			_log->Info("Video memory budget set to {0}GB", videoMemoryBudgetInGB);
		}

		// Wait for the memory budget to change, or a worker thread shutdown request.
		DWORD waitForMultipleObjectsReturn = WaitForMultipleObjects(2, &eventHandles[0], FALSE, INFINITE);
		if (waitForMultipleObjectsReturn != WAIT_OBJECT_0)
		{
			shutdownRequested = true;
			continue;
		}
	}

	// Unregister the change notification
	_adapter->UnregisterVideoMemoryBudgetChangeNotification(videoMemoryBudgetChangeEventRegistrationCookie);

	// Dispose of our event handle
	CloseHandle(videoMemoryBudgetChangeEvent);

	// Notify the calling thread that this worker thread has now shut down
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_notifyVideoMemoryBudgetChangeWorkerStopped.notify_all();
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
			PerformRenderOperation();
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
	// Reset the draw command allocator to reclaim memory
	if (FAILED(_drawCommandAllocator->Reset()))
	{
		_log->Error("Failed to reset draw command allocator");
	}

	// Transition our build state to our draw state
	std::swap(_buildIndex, _drawIndex);
	_state[_buildIndex].renderPasses = _state[_drawIndex].renderPasses;
	_state[_buildIndex].migrateStatePendingObjects.clear();
	_state[_buildIndex].bufferUpdatePendingObjects.clear();
	_state[_buildIndex].bufferTransferPendingObjects.clear();

	// Insert a fence to signal when our build commands have finished executing
	if (FAILED(_buildCommandQueue->Signal(_buildFence.Get(), 1)))
	{
		_log->Error("Failed to signal build fence");
	}

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

	// Wait for the build fence to be reached
	WaitForSingleObject(_buildFenceEvent, INFINITE);

	// Reset our command allocators from our build command list pool. Note that this one place in code, after we've
	// waited on the build fence above but before we return from this function, is the only place we can guarantee that
	// all command queues are idle. At any other point there are potentially build and/or draw tasks in progress. As
	// soon as we return from this function the draw process will be kicked off, and build tasks for the next frame may
	// also begin to be submitted in parallel.
	_buildCommandListPool->ResetDirtyCommandAllocators();

	// If any temporary transfer buffers were allocated to be freed during this new frame, destroy them now.
	_state[_drawIndex].transferBufferAllocations.clear();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformRenderOperation()
{
	// Reset our build fence
	if (FAILED(_buildFence->Signal(0)))
	{
		_log->Error("Failed to reset build fence");
	}

	// Configure the fence to raise our event when it is reached
	if (FAILED(_buildFence->SetEventOnCompletion(1, _buildFenceEvent)))
	{
		_log->Error("Failed to raise event on build fence completion");
	}

	// Calculate the metrics we'll use to divide up the render tasks into separate command lists. There are two
	// competing priorities we need to worry about in a command list, residency requirements and total vertex count.
	// Residency is a vital concern. A command list can't be split into smaller parts once it's created, and the set of
	// resources being used by commands in that list determines the memory requirements at the time it is executed. If
	// we require more video memory to execute a list than is currently available, it can lead to extremely poor
	// performance or failure to execute correctly. To manage residency, we use the D3D12 Residency Starter Library from
	// Microsoft:
	// https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12Residency
	// This manages the actual task of evicting resources and making them resident again at the required times. In order
	// for this library to be able to do its work though, we need to divide execution into command lists which don't
	// require too much memory. To manage that task, we track our video memory budget that's been allocated to the
	// process, and aim for no more than half of that being required for a given command list. That handles our
	// residency requirements. There are also some performance considerations too however. If we end up with one big
	// command list, our rendering performance is sub-optimal, as no rendering will begin until we've fully traversed
	// all renderable objects. We want rendering to commence earlier. Creating too many command lists with too little
	// workload will also result in sub-optimal rendering however, as there's overhead for each call to execute a
	// command list. To manage this, we incorporate some simple heuristics and attempt to follow the advice from NVidia
	// here:
	// https://developer.nvidia.com/dx12-dos-and-donts
	// It states that we should "Try to aim at a reasonable number of command lists in the range of 15-30 or below. Try
	// to bundle those CLs into 5-10 ExecuteCommandLists() calls per frame". There's no real advantage to us here
	// combining several command lists into one ExecuteCommandLists call. We therefore aim for an average of 5-7
	// ExecuteCommandLists calls for the draw process, and a maximum of 10, unless residency requirements necessitate
	// more. This should leave us well positioned to get good performance out of the rendering process.
	size_t commandListTargetMaxIndices = std::max((_totalVerticesLastFrame / 5), MinTargetIndicesPerCommandList);
	size_t residencySetTargetSizeInBytes = std::max((_videoMemoryBudget / 2), MinResidencySetTargetInBytes);
	size_t commandListTargetMaxLists = 10;

	// Open a command list and residency set for the draw process
	AllocateDrawCommandList();
	ID3D12GraphicsCommandList* drawCommandList = _drawCommandList.Get();
	D3DX12Residency::ResidencySet* residencySet = _drawResidencySet;
	size_t residencySetSizeInBytes = 0;
	size_t commandListsSubmittted = 0;

	// Push a render marker if required
	if (_useRenderMarkers)
	{
		EmitBeginEvent(drawCommandList, SetupMarkerColor(), L"Setup");
	}

	// Initiate any pending data transfer operations within GPU memory
	for (const auto& entry : _state[_drawIndex].bufferTransferPendingObjects)
	{
		// Complete any pending data transfers for the target object
		residencySetSizeInBytes += std::visit([drawCommandList, residencySet](auto* object) { return object->CompletePendingDataTransfers(drawCommandList, residencySet); }, entry);

		// If the current command list has reached our limits, submit it now, and create a new command list.
		if (residencySetSizeInBytes > residencySetTargetSizeInBytes)
		{
			++commandListsSubmittted;
			SubmitDrawCommandList();
			AllocateDrawCommandList();
			drawCommandList = _drawCommandList.Get();
			residencySet = _drawResidencySet;
			residencySetSizeInBytes = 0;
		}
	}

	// Initiate any pending data transfer operations from CPU to GPU memory
	for (const auto& entry : _state[_drawIndex].bufferUpdatePendingObjects)
	{
		// Complete any pending data writes for the target object
		residencySetSizeInBytes += std::visit([drawCommandList, residencySet](auto* object) { return object->CompletePendingDataWrites(drawCommandList, residencySet); }, entry);

		// If the current command list has reached our limits, submit it now, and create a new command list.
		if (residencySetSizeInBytes > residencySetTargetSizeInBytes)
		{
			++commandListsSubmittted;
			SubmitDrawCommandList();
			AllocateDrawCommandList();
			drawCommandList = _drawCommandList.Get();
			residencySet = _drawResidencySet;
			residencySetSizeInBytes = 0;
		}
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
	// Note that unlike Direct3D 11, where it's relatively easy to fall back to changing our constant buffer state
	// partway through draw calls, under Direct3D 12 this doesn't work automatically. We'd have to properly stage each
	// write and add use fences to synchronize our draw calls with our state changes, which would be very painful to do
	// manually, and almost certainly slower than what was achieved automatically for us under Direct3D 11.
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
				bool hasGroupStateValues = false;
				bool checkedGroupStateValues = false;
				size_t stateBucketCount = groupNode->GetStateBucketCount();
				for (size_t stateBucketID = 0; stateBucketID < stateBucketCount; ++stateBucketID)
				{
					// If this state bucket is empty, skip it.
					const std::vector<Direct3DRenderableNode*>& renderableNodes = groupNode->GetChildNodes(stateBucketID);
					bool isComputeTask = groupNode->HasComputeTask();
					if (!isComputeTask && renderableNodes.empty())
					{
						continue;
					}

					// If we haven't latched the shader program yet, do it now. We defer this process to minimize work
					// if all the state nodes for a given program node are empty.
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
						shaderProgram->BeginGlobalConstantBufferBuildingSession(drawCommandList, constantBufferBuildingSession);
					}

					// If this state group node sets global constant state, load it now.
					if (!checkedGroupStateValues)
					{
						const auto& stateValueEntriesForGroupNode = groupNode->GetValueEntries();
						hasGroupStateValues = !stateValueEntriesForGroupNode.empty();
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
						checkedGroupStateValues = true;
					}

					// If this group node is performing a compute task, associate the current constant buffer state with the
					// group node.
					if (isComputeTask)
					{
						// Bind the generated global constant data to the target renderable
						shaderProgram->GenerateGlobalConstantBufferBindings(drawCommandList, constantBufferBuildingSession, groupNode->GetGlobalConstantBufferBindingInfo());
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
						shaderProgram->GenerateGlobalConstantBufferBindings(drawCommandList, constantBufferBuildingSession, renderableNode->GetGlobalConstantBufferBindingInfo());

						// If we changed state values for this renderable, restore it to the pushed state baseline.
						if (hasRenderableStateValues)
						{
							shaderProgram->RestoreGlobalConstantBufferBaseline();
						}
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
				shaderProgram->CompleteGlobalConstantBufferBuildingSession(drawCommandList, constantBufferBuildingSession);
				shaderProgram->PopGlobalConstantBufferState();
			}
		}
	}

	// Pop a render marker if required
	if (_useRenderMarkers)
	{
		EmitEndEvent(drawCommandList);
	}

	// Submit our current command list to get all pending transfers started. We need these to all complete before draw
	// operations can begin, so there's no advantage in waiting any longer.
	++commandListsSubmittted;
	SubmitDrawCommandList();
	AllocateDrawCommandList();
	drawCommandList = _drawCommandList.Get();
	residencySet = _drawResidencySet;
	residencySetSizeInBytes = 0;
	size_t commandListCurrentVertexCount = 0;
	size_t totalVertexCount = 0;
	size_t commandListCurrentDrawCallCount = 0;
	size_t commandListTargetMaxDrawCalls = 1000;

	// Bind our descriptor heaps
	const std::vector<ID3D12DescriptorHeap*>& descriptorHeaps = _heapManager->GetShaderVisibleDescriptorHeaps();
	if (!descriptorHeaps.empty())
	{
		drawCommandList->SetDescriptorHeaps((UINT)descriptorHeaps.size(), descriptorHeaps.data());
	}

	// Traverse the render tree and perform our draw operations
	_captureTargetsPresent = false;
	_boundWindowFramebuffers.clear();
	_boundTextureFramebuffers.clear();
	_boundDataArrays.clear();
	_boundTexelArrays.clear();
	Direct3DShaderProgram* currentShaderProgram = nullptr;
	Direct3DFrameBuffer* currentFramebuffer = nullptr;
	bool boundShaderProgram = false;
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
			EmitBeginEvent(drawCommandList, RenderPassMarkerColor(), renderPassNode->DebugName());
		}

		// Bind the framebuffer for this render pass
		Direct3DFrameBuffer* newFramebuffer = renderPassNode->GetFrameBuffer();
		if (newFramebuffer != currentFramebuffer)
		{
			if (currentFramebuffer != nullptr)
			{
				currentFramebuffer->UnbindFrameBuffer(drawCommandList);
			}
			if (newFramebuffer != nullptr)
			{
				newFramebuffer->BindFrameBuffer(drawCommandList, _drawCommandQueue.Get(), _dxgiFactory.Get());
				if (newFramebuffer->IsBoundToWindow())
				{
					_boundWindowFramebuffers.push_back(newFramebuffer);
				}
				else
				{
					_boundTextureFramebuffers.push_back(newFramebuffer);
				}
				_captureTargetsPresent |= newFramebuffer->HasCaptureTargets();
			}
			currentFramebuffer = newFramebuffer;
		}

		// Apply fixed state from this render pass node
		renderPassNode->ApplyFixedState(drawCommandList);

		// Render each child node
		const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const Direct3DRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			bool markedProgram = false;

			// If there are no child nodes in this program node, skip it.
			Direct3DProgramNode* programNode = childNodeEntry.node;
			const std::vector<Direct3DStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			if (groupNodes.empty())
			{
				continue;
			}
			int frameBufferIndex = childNodeEntry.frameBufferIndex;

			// Select the shader program to use for this program node
			Direct3DShaderProgram* shaderProgram = programNode->GetShaderProgram();
			if (shaderProgram != currentShaderProgram)
			{
				currentShaderProgram = shaderProgram;
				boundShaderProgram = false;
			}

			// Retrieve any any textures or state buffer entries set as defaults at the render pass level. Note that these
			// cannot actually be bound yet, as the bindings can only be performed after the root signature has been set.
			bool boundDefaultRenderPassState = false;
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

			// Render each child node
			for (Direct3DStateGroupNode* groupNode : groupNodes)
			{
				bool markedGroup = false;

				// Render nodes from each state bucket in the group node
				bool isComputeTask = groupNode->HasComputeTask();
				size_t stateBucketCount = groupNode->GetStateBucketCount();
				for (size_t stateBucketID = 0; stateBucketID < stateBucketCount; ++stateBucketID)
				{
					// If this state bucket is empty, skip it.
					const std::vector<Direct3DRenderableNode*>& renderableNodes = groupNode->GetChildNodes(stateBucketID);
					if (!isComputeTask && renderableNodes.empty())
					{
						continue;
					}

					// Push a render marker if required
					if (_useRenderMarkers)
					{
						// If program or group hasn't already been started, start marker for it now
						if (!markedProgram)
						{
							EmitBeginEvent(drawCommandList, ProgramMarkerColor(), programNode->DebugName());
							markedProgram = true;
						}
						if (!markedGroup)
						{
							EmitBeginEvent(drawCommandList, GroupMarkerColor(), groupNode->DebugName());
							markedGroup = true;
						}

						// Push the render marker
						EmitBeginEvent(drawCommandList, PipelineMarkerColor(), L"Pipeline");
					}

					// Bind the shader program if required
					if (!boundShaderProgram)
					{
						shaderProgram->BindShaderProgram(drawCommandList);
						boundShaderProgram = true;
					}

					// Apply fixed state associated with this state group node
					groupNode->ApplyFixedState(stateBucketID, frameBufferIndex, drawCommandList, shaderProgram, currentFramebuffer);

					// Bind any textures or state buffer entries set as defaults at the render pass level. Note that we need to do
					// this after the root signature has been set, so it must occur after the "ApplyFixedState" call on the group
					// node above.
					if (!boundDefaultRenderPassState)
					{
						if (hasDefaultTextureEntries)
						{
							BindTextures(drawCommandList, *defaultTextureEntries, shaderProgram, isComputeTask);
						}
						if (hasDefaultSamplerEntries)
						{
							BindSamplers(drawCommandList, *defaultSamplerEntries, shaderProgram, isComputeTask);
						}
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(drawCommandList, *defaultStateBufferEntries, shaderProgram, isComputeTask);
						}
						if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(drawCommandList, *defaultResourceBufferEntries, shaderProgram, true, isComputeTask);
						}
						boundDefaultRenderPassState = true;
					}

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
							BindTextures(drawCommandList, *defaultTextureEntries, shaderProgram, isComputeTask);
						}
						BindTextures(drawCommandList, textureEntriesForGroupNode, shaderProgram, isComputeTask);
					}
					if (hasSamplerEntriesForStateGroupNode)
					{
						if (hasDefaultSamplerEntries)
						{
							BindSamplers(drawCommandList, *defaultSamplerEntries, shaderProgram, isComputeTask);
						}
						BindSamplers(drawCommandList, samplerEntriesForGroupNode, shaderProgram, isComputeTask);
					}
					if (hasStateBufferEntriesForStateGroupNode)
					{
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(drawCommandList, *defaultStateBufferEntries, shaderProgram, isComputeTask);
						}
						BindStateBuffers(drawCommandList, stateBufferEntriesForGroupNode, shaderProgram, isComputeTask);
					}
					if (hasResourceBufferEntriesForStateGroupNode)
					{
						if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(drawCommandList, *defaultResourceBufferEntries, shaderProgram, false, isComputeTask);
						}
						BindResourceArrays(drawCommandList, resourceBufferEntriesForGroupNode, shaderProgram, true, isComputeTask);
					}

					// If this group node is performing a compute task, execute it now.
					if (isComputeTask)
					{
						// Attach the required global constant buffer entries
						shaderProgram->ApplyGlobalConstantBufferBindings(drawCommandList, groupNode->GetGlobalConstantBufferBindingInfo());

						// Perform the compute task
						auto threadGroupCounts = groupNode->GetComputeThreadGroupCounts();
						drawCommandList->Dispatch(threadGroupCounts.X(), threadGroupCounts.Y(), threadGroupCounts.Z());
					}

					// Render each child node
					for (Direct3DRenderableNode* renderableNode : renderableNodes)
					{
						// Push a render marker if required
						if (_useRenderMarkers)
						{
							EmitBeginEvent(drawCommandList, RenderableMarkerColor(), renderableNode->DebugName());
						}

						// If we need to spin out a new command list and residency set, do it now.
						if (((commandListsSubmittted < commandListTargetMaxLists) && (commandListCurrentVertexCount > commandListTargetMaxIndices)) || (residencySetSizeInBytes > residencySetTargetSizeInBytes) || (commandListCurrentDrawCallCount > commandListTargetMaxDrawCalls))
						{
							// Submit the current command list and allocate another one
							++commandListsSubmittted;
							SubmitDrawCommandList();
							AllocateDrawCommandList();
							drawCommandList = _drawCommandList.Get();
							residencySet = _drawResidencySet;
							residencySetSizeInBytes = 0;
							commandListCurrentVertexCount = 0;
							commandListCurrentDrawCallCount = 0;

							// Bind our descriptor heaps
							if (!descriptorHeaps.empty())
							{
								drawCommandList->SetDescriptorHeaps((UINT)descriptorHeaps.size(), descriptorHeaps.data());
							}

							// Bind the current framebuffer
							currentFramebuffer->BindFrameBuffer(drawCommandList, _drawCommandQueue.Get(), _dxgiFactory.Get(), false);

							// Bind the shader program
							shaderProgram->BindShaderProgram(drawCommandList);

							// Apply fixed state associated with this state group node
							groupNode->ApplyFixedState(stateBucketID, frameBufferIndex, drawCommandList, shaderProgram, currentFramebuffer);

							// Restore any initial bindings
							if (hasDefaultTextureEntries)
							{
								BindTextures(drawCommandList, *defaultTextureEntries, shaderProgram, false);
							}
							if (hasTextureEntriesForStateGroupNode)
							{
								BindTextures(drawCommandList, textureEntriesForGroupNode, shaderProgram, false);
							}
							if (hasDefaultSamplerEntries)
							{
								BindSamplers(drawCommandList, *defaultSamplerEntries, shaderProgram, false);
							}
							if (hasSamplerEntriesForStateGroupNode)
							{
								BindSamplers(drawCommandList, samplerEntriesForGroupNode, shaderProgram, false);
							}
							if (hasDefaultStateBufferEntries)
							{
								BindStateBuffers(drawCommandList, *defaultStateBufferEntries, shaderProgram, false);
							}
							if (hasStateBufferEntriesForStateGroupNode)
							{
								BindStateBuffers(drawCommandList, stateBufferEntriesForGroupNode, shaderProgram, false);
							}
							if (hasDefaultResourceBufferEntries)
							{
								BindResourceArrays(drawCommandList, *defaultResourceBufferEntries, shaderProgram, false, false);
							}
							if (hasResourceBufferEntriesForStateGroupNode)
							{
								BindResourceArrays(drawCommandList, resourceBufferEntriesForGroupNode, shaderProgram, false, false);
							}
						}

						// Add the resources for this object into the residency set, and update our running count of
						// used resources in the current command list. Note that currently, we only track vertex and
						// index buffers for residency purposes. We aim for only a 50% memory utilization target from
						// the draw process, so smaller objects like state buffers are considered inconsequential for
						// tracking purposes. We also currently ignore texture resources however, where if there were a
						// large number of textures of significant size, we may want to factor these in. At the current
						// stage though, based on our usage requirements, tracking textures would involve additional
						// rendering overhead which isn't considered to be worth taking on.
						//##TODO## Review residency tracking now that data arrays are being added. Data arrays
						//have the potential to be extremely large, as can texture resources. We need to start tracking
						//residency requirements with these resources.
						residencySetSizeInBytes += renderableNode->AddResourcesToResidencySet(residencySet);
						size_t objectVertexCount = renderableNode->GetTotalVertexCount();
						commandListCurrentVertexCount += objectVertexCount;
						totalVertexCount += objectVertexCount;
						++commandListCurrentDrawCallCount;

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
							BindTextures(drawCommandList, textureEntriesForRenderableNode, shaderProgram, false);
						}
						if (hasSamplerEntries)
						{
							BindSamplers(drawCommandList, samplerEntriesForRenderableNode, shaderProgram, false);
						}
						if (hasStateBufferEntries)
						{
							BindStateBuffers(drawCommandList, stateBufferEntriesForRenderableNode, shaderProgram, false);
						}
						if (hasResourceBufferEntries)
						{
							BindResourceArrays(drawCommandList, resourceBufferEntriesForRenderableNode, shaderProgram, true, false);
						}

						// Attach the required global constant buffer entries
						shaderProgram->ApplyGlobalConstantBufferBindings(drawCommandList, renderableNode->GetGlobalConstantBufferBindingInfo());

						// Draw this renderable object
						renderableNode->Draw(drawCommandList, shaderProgram);

						// If we bound any textures or state buffer entries, and the state group node provided some
						// initial bindings of its own, restore the bindings that were set on the state group node now.
						if (hasTextureEntries)
						{
							if (hasDefaultTextureEntries)
							{
								BindTextures(drawCommandList, *defaultTextureEntries, shaderProgram, false);
							}
							if (hasTextureEntriesForStateGroupNode)
							{
								BindTextures(drawCommandList, textureEntriesForGroupNode, shaderProgram, false);
							}
						}
						if (hasSamplerEntries)
						{
							if (hasDefaultSamplerEntries)
							{
								BindSamplers(drawCommandList, *defaultSamplerEntries, shaderProgram, false);
							}
							if (hasSamplerEntriesForStateGroupNode)
							{
								BindSamplers(drawCommandList, samplerEntriesForGroupNode, shaderProgram, false);
							}
						}
						if (hasStateBufferEntries)
						{
							if (hasDefaultStateBufferEntries)
							{
								BindStateBuffers(drawCommandList, *defaultStateBufferEntries, shaderProgram, false);
							}
							if (hasStateBufferEntriesForStateGroupNode)
							{
								BindStateBuffers(drawCommandList, stateBufferEntriesForGroupNode, shaderProgram, false);
							}
						}
						if (hasResourceBufferEntries)
						{
							if (hasDefaultResourceBufferEntries)
							{
								BindResourceArrays(drawCommandList, *defaultResourceBufferEntries, shaderProgram, false, false);
							}
							if (hasStateBufferEntriesForStateGroupNode)
							{
								BindResourceArrays(drawCommandList, resourceBufferEntriesForGroupNode, shaderProgram, false, false);
							}
						}

						// Pop a render marker if required
						if (_useRenderMarkers)
						{
							EmitEndEvent(drawCommandList);
						}
					}

					// Pop a render marker if required
					if (_useRenderMarkers)
					{
						EmitEndEvent(drawCommandList);
					}
				}

				// Pop a render marker if required
				if (_useRenderMarkers && markedGroup)
				{
					EmitEndEvent(drawCommandList);
				}
			}

			// Pop a render marker if required
			if (_useRenderMarkers && markedProgram)
			{
				EmitEndEvent(drawCommandList);
			}
		}

		// Perform any unbind operations for the render pass node
		renderPassNode->PerformUnbindOperations(drawCommandList);

		// Pop a render marker if required
		if (_useRenderMarkers)
		{
			EmitEndEvent(drawCommandList);
		}
	}

	// Unbind any framebuffer which may still be bound
	if (currentFramebuffer != nullptr)
	{
		currentFramebuffer->UnbindFrameBuffer(drawCommandList);
	}

	// Submit the last command list for drawing
	SubmitDrawCommandList();

	// Update our count of the total number of indices in the last rendered frame
	_totalVerticesLastFrame = totalVertexCount;

	// If we have at least one capture target, begin the capture process now.
	_capturedFramebufferOutputsInCurrentFrame.clear();
	_capturedDataArrayOutputsInCurrentFrame.clear();
	_capturedTexelArrayOutputsInCurrentFrame.clear();
	if (_captureTargetsPresent)
	{
		AllocateDrawCommandList();
		for (Direct3DFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(_drawCommandList.Get());
		}
		for (Direct3DFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(_drawCommandList.Get());
		}
		for (Direct3DDataArray* resourceBuffer : _boundDataArrays)
		{
			resourceBuffer->CaptureDataBufferOutput(_drawCommandList.Get());
		}
		for (Direct3DTexelArray* resourceBuffer : _boundTexelArrays)
		{
			resourceBuffer->CaptureDataBufferOutput(_drawCommandList.Get());
		}
		SubmitDrawCommandList();
	}

	// Insert a fence to signal when our draw commands have finished executing
	if (FAILED(_drawCommandQueue->Signal(_drawFence.Get(), 1)))
	{
		_log->Error("Failed to signal draw fence");
	}

	// Forward any pending debug messages to our logging system if required
	if (_enableDebugLogging)
	{
		ProcessPendingDebugMessages();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::EmitBeginEvent(ID3D12GraphicsCommandList* commandList, uint32_t color, const std::wstring& name) const
{
#ifdef USE_PIX
	PIXBeginEvent(commandList, color, name.c_str());
#else
	commandList->BeginEvent(0, name.c_str(), (UINT)name.size());
#endif
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::EmitEndEvent(ID3D12GraphicsCommandList* commandList) const
{
#ifdef USE_PIX
	PIXEndEvent(commandList);
#else
	commandList->EndEvent();
#endif
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::PerformSwapOperation()
{
	// Wait for the draw fence to be reached
	WaitForSingleObject(_drawFenceEvent, INFINITE);

	// Reset our draw fence
	if (FAILED(_drawFence->Signal(0)))
	{
		_log->Error("Failed to reset draw fence");
	}

	// Configure the fence to raise our event when it is reached
	if (FAILED(_drawFence->SetEventOnCompletion(1, _drawFenceEvent)))
	{
		_log->Error("Failed to raise event on draw fence completion");
	}

	// Swap the window buffers to the screen
	for (Direct3DFrameBuffer* frameBuffer : _boundWindowFramebuffers)
	{
		frameBuffer->PresentToWindow();
	}

	// If we have at least one capture target, complete the capture process.
	if (_captureTargetsPresent)
	{
		for (Direct3DFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
		for (Direct3DFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
		for (Direct3DDataArray* resourceBuffer : _boundDataArrays)
		{
			resourceBuffer->CompleteCaptureDataBufferOutput();
		}
		for (Direct3DTexelArray* resourceBuffer : _boundTexelArrays)
		{
			resourceBuffer->CompleteCaptureDataBufferOutput();
		}
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
void Direct3DRenderer::BindTextures(ID3D12GraphicsCommandList* commandList, const std::vector<ITextureBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindTexture(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindSamplers(ID3D12GraphicsCommandList* commandList, const std::vector<ISamplerBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (ISamplerBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindSampler(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindStateBuffers(ID3D12GraphicsCommandList* commandList, const std::vector<StateBufferBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindStateBuffer(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::BindResourceArrays(ID3D12GraphicsCommandList* commandList, const std::vector<ResourceArrayBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool performReset, bool computeShaderBinding)
{
	for (ResourceArrayBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindResourceArray(this, program, commandList, performReset, computeShaderBinding);
		_captureTargetsPresent |= bindingInfo->HasCaptureTargets();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindTextures(ID3D12GraphicsCommandList* commandList, const std::vector<ITextureBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindTexture(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindSamplers(ID3D12GraphicsCommandList* commandList, const std::vector<ISamplerBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (ISamplerBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindSampler(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindStateBuffers(ID3D12GraphicsCommandList* commandList, const std::vector<StateBufferBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindStateBuffer(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::UnbindResourceArrays(ID3D12GraphicsCommandList* commandList, const std::vector<ResourceArrayBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding)
{
	for (ResourceArrayBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindResourceArray(this, program, commandList, computeShaderBinding);
	}
}

//----------------------------------------------------------------------------------------
// Resource methods
//----------------------------------------------------------------------------------------
ID3D12Device* Direct3DRenderer::GetDevice() const
{
	return _device.Get();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::EnsureHeapExists(Direct3DHeapManager::ResourceType type) const
{
	std::lock_guard<std::mutex> lock(_heapAllocationMutex);
	_heapManager->PreAllocateHeapForType(type);
}

//----------------------------------------------------------------------------------------
std::unique_ptr<DescriptorHandle> Direct3DRenderer::AllocateDescriptor(Direct3DHeapManager::ResourceType type) const
{
	std::lock_guard<std::mutex> lock(_heapAllocationMutex);
	return _heapManager->AllocateDescriptor(type);
}

//----------------------------------------------------------------------------------------
CommandListHandle Direct3DRenderer::GetBuildCommandList() const
{
	return _buildCommandListPool->AllocateCommandList(true);
}

//----------------------------------------------------------------------------------------
ID3D12CommandQueue* Direct3DRenderer::GetBuildCommandQueue() const
{
	return _buildCommandQueue.Get();
}

//----------------------------------------------------------------------------------------
ID3D12CommandQueue* Direct3DRenderer::GetDrawCommandQueue() const
{
	return _drawCommandQueue.Get();
}

//----------------------------------------------------------------------------------------
D3DX12Residency::ResidencyManager& Direct3DRenderer::ResidencyManager() const
{
	return *_residencyManager;
}

//----------------------------------------------------------------------------------------
D3D12MA::Allocator& Direct3DRenderer::MemoryManager() const
{
	return *_memoryManager;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::AllocateDrawCommandList()
{
	// Reset our draw command list
	if (FAILED(_drawCommandList->Reset(_drawCommandAllocator.Get(), nullptr)))
	{
		_log->Error("Failed to reset draw command list");
	}

	// Re-open our draw residency set. Note that there's no explicit reset method for residency sets, but they can be
	// reused after being closed.
	if (FAILED(_drawResidencySet->Open()))
	{
		_log->Error("Failed to open draw residency set");
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::SubmitDrawCommandList()
{
	// Close our current draw command list and residency set
	auto* drawCommandList = _drawCommandList.Get();
	auto* residencySet = _drawResidencySet;
	HRESULT drawCommandListCloseResult = drawCommandList->Close();
	if (FAILED(drawCommandListCloseResult))
	{
		_log->Error("Failed to close draw command list with error code {0}", drawCommandListCloseResult);
	}
	HRESULT residencySetCloseResult = residencySet->Close();
	if (FAILED(residencySetCloseResult))
	{
		_log->Error("Failed to close draw residency set with error code {0}", residencySetCloseResult);
	}

	// Submit the completed command list for execution
	ID3D12CommandList* commandListPointer = drawCommandList;
	_residencyManager->ExecuteCommandLists(_drawCommandQueue.Get(), &commandListPointer, &residencySet, 1);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::CreatePersistentUploadBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation)
{
	CreatePersistentTransferBuffer(bufferSizeInBytes, bufferPointer, bufferAllocation, true);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::CreatePersistentReadbackBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation)
{
	CreatePersistentTransferBuffer(bufferSizeInBytes, bufferPointer, bufferAllocation, false);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::CreatePersistentTransferBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation, bool isUploadBuffer)
{
	// Create the requested transfer buffer
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSizeInBytes);
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
	allocationDesc.HeapType = (isUploadBuffer ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_READBACK);
	D3D12_RESOURCE_STATES initialResourceState = (isUploadBuffer ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST);
	std::unique_lock<std::mutex> lock(_transferBufferMutex);
	HRESULT createResourceResult = _memoryManager->CreateResource(&allocationDesc, &resourceDescription, initialResourceState, nullptr, &bufferAllocation, IID_PPV_ARGS(&bufferPointer));
	lock.unlock();
	if (FAILED(createResourceResult))
	{
		_log->Error("CreateResource failed with code {0}", createResourceResult);
		return;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::CreateTemporaryUploadBuffer(size_t bufferSizeInBytes, ID3D12Resource*& buffer, bool keepUntilNextFrame)
{
	// Initialize our outputs
	buffer = nullptr;

	// Create the requested transfer buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferPointer;
	D3D12MA::Allocation* bufferAllocation;
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSizeInBytes);
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	std::unique_lock<std::mutex> lock(_transferBufferMutex);
	HRESULT createResourceResult = _memoryManager->CreateResource(&allocationDesc, &resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &bufferAllocation, IID_PPV_ARGS(&bufferPointer));
	lock.unlock();
	if (FAILED(createResourceResult))
	{
		_log->Error("CreateResource failed with code {0}", createResourceResult);
		return;
	}
	buffer = bufferPointer.Get();

	// Add this buffer to the set of allocated transfer buffers
	TransferBufferAllocation allocationEntry{};
	allocationEntry.bufferSizeInBytes = bufferSizeInBytes;
	allocationEntry.buffer = std::move(bufferPointer);
	allocationEntry.bufferAllocation = std::shared_ptr<D3D12MA::Allocation>(bufferAllocation, [](D3D12MA::Allocation* allocation) {
		if (allocation != nullptr)
		{
			allocation->Release();
		}
	});
	lock.lock();
	if (keepUntilNextFrame)
	{
		_state[_drawIndex].transferBufferAllocations.push_back(std::move(allocationEntry));
	}
	else
	{
		_state[_buildIndex].transferBufferAllocations.push_back(std::move(allocationEntry));
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::TrackTemporaryUploadBufferUntilNextFrame(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, std::shared_ptr<D3D12MA::Allocation>& bufferAllocation)
{
	TransferBufferAllocation allocationEntry{};
	allocationEntry.buffer = std::move(buffer);
	allocationEntry.bufferAllocation = std::move(bufferAllocation);

	std::scoped_lock<std::mutex> lock(_transferBufferMutex);
	_state[_drawIndex].transferBufferAllocations.push_back(std::move(allocationEntry));
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::ExtendTransferBufferLifetimeToNextFrame(ID3D12Resource* buffer)
{
	std::scoped_lock<std::mutex> lock(_transferBufferMutex);
	auto& buildTransferBuffers = _state[_buildIndex].transferBufferAllocations;
	for (auto entryIterator = buildTransferBuffers.begin(); entryIterator != buildTransferBuffers.end(); ++entryIterator)
	{
		if (entryIterator->buffer.Get() == buffer)
		{
			_state[_drawIndex].transferBufferAllocations.push_back(std::move(*entryIterator));
			buildTransferBuffers.erase(entryIterator);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
// Settings methods
//----------------------------------------------------------------------------------------
bool Direct3DRenderer::DebugLoggingEnabled() const
{
	return _enableDebugLogging;
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
bool Direct3DRenderer::IsFeaturePresent(DXGI_FEATURE feature) const
{
	// Try and get the IDXGIFactory5 interface. If that's not available, the feature definitely isn't either.
	IDXGIFactory5* _dxgiFactory5 = nullptr;
	if (FAILED(_dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(&_dxgiFactory5))))
	{
		return false;
	}

	// Check if the feature is supported
	BOOL featureSupported = FALSE;
	HRESULT checkFeatureSupportReturn = _dxgiFactory5->CheckFeatureSupport(feature, &featureSupported, sizeof(featureSupported));
	if (FAILED(checkFeatureSupportReturn))
	{
		return false;
	}
	return (featureSupported == TRUE);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderer::HighestSupportedShaderModel(unsigned int& targetShaderModelMajor, unsigned int& targetShaderModelMinor) const
{
	// If the highest supported shader model value isn't already cached, retrieve it.
	std::scoped_lock<std::mutex> lock(_cachedFeatureMutex);
	if (!_cachedMaxShaderModel)
	{
		D3D12_FEATURE_DATA_SHADER_MODEL featureData = {};
		featureData.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;
		HRESULT checkFeatureSupportResult = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &featureData, sizeof(featureData));
		if (FAILED(checkFeatureSupportResult))
		{
			_maxShaderModelMajor = 5;
			_maxShaderModelMinor = 1;
		}
		else
		{
			_maxShaderModelMajor = featureData.HighestShaderModel >> 4;
			_maxShaderModelMinor = (featureData.HighestShaderModel & 0xF);
		}
		_cachedMaxShaderModel = true;
	}

	// Return the cached data to the caller
	targetShaderModelMajor = _maxShaderModelMajor;
	targetShaderModelMinor = _maxShaderModelMinor;
}

} // namespace cobalt::graphics
