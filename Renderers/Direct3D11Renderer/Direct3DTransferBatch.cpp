// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DTransferBatch.h"
#include "Direct3DRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTransferBatch::Direct3DTransferBatch(cobalt::logging::ILogger* log, Direct3DRenderer* renderer, StartTiming startTiming, EndTiming endTiming)
: _log(log), _renderer(renderer), _startTiming(startTiming), _endTiming(endTiming)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTransferBatch::Delete()
{
	// It's permissible to submit a batch without waiting for it to complete, and allowing the transfer batch object
	// itself to be cleaned up. If we do this though, we need to clean up resources for this pending transfer. In this
	// case, we defer the actual deletion of this object, and spawn a thread here to wait for the batch transfer to
	// complete, cleaning up this object when it is finished.
	if (!IsComplete())
	{
		std::thread([=]() {
			if (!_isSubmitted && (_usageCount != 0))
			{
				_log->Error("Transfer batch was destroyed without being submitted, but after it has been used for operations.");
				SubmitBatch();
			}
			WaitForComplete();
			delete this;
		})
		  .detach();
		return;
	}

	// Since this batch is completed, delete it immediately.
	delete this;
}

//----------------------------------------------------------------------------------------
// Submission methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DTransferBatch::SubmitBatch()
{
	std::unique_lock<std::mutex> lock(_completeMutex);
	if (_isSubmitted)
	{
		_log->Error("Transfer batch has already been submitted.");
		return false;
	}
	_isSubmitted = true;
	if (_usageCount == 0)
	{
		NotifyComplete();
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DTransferBatch::IsSubmitted() const
{
	return _isSubmitted;
}

//----------------------------------------------------------------------------------------
bool Direct3DTransferBatch::IsComplete() const
{
	return _isComplete;
}

//----------------------------------------------------------------------------------------
void Direct3DTransferBatch::WaitForComplete() const
{
	std::unique_lock<std::mutex> lock(_completeMutex);
	if (!_isComplete)
	{
		_notifyComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTransferBatch::IncrementUsageCount()
{
	std::unique_lock<std::mutex> lock(_completeMutex);
	++_usageCount;
}

//----------------------------------------------------------------------------------------
void Direct3DTransferBatch::DecrementUsageCount()
{
	std::unique_lock<std::mutex> lock(_completeMutex);
	if (--_usageCount == 0)
	{
		NotifyComplete();
	}
}

//----------------------------------------------------------------------------------------
// Submission methods
//----------------------------------------------------------------------------------------
void Direct3DTransferBatch::NotifyComplete()
{
	_isComplete = true;
	_notifyComplete.notify_all();
}

} // namespace cobalt::graphics
