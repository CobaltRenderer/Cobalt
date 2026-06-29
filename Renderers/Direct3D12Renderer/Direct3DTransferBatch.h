// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <condition_variable>
#include <thread>
namespace cobalt::graphics {
class Direct3DRenderer;

class Direct3DTransferBatch : public ITransferBatch
{
public:
	// Constructors
	Direct3DTransferBatch(cobalt::logging::ILogger* log, Direct3DRenderer* renderer, StartTiming startTiming, EndTiming endTiming);

	// Initialization methods
	void Delete() override;

	// Submission methods
	SuccessToken SubmitBatch() override;
	bool IsSubmitted() const override;
	bool IsComplete() const override;
	void WaitForComplete() const override;
	void IncrementUsageCount();
	void DecrementUsageCount();

private:
	// Submission methods
	void NotifyComplete();

private:
	logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	StartTiming _startTiming;
	EndTiming _endTiming;
	unsigned int _usageCount = 0;
	std::atomic<bool> _isComplete = false;
	std::atomic<bool> _isSubmitted = false;
	mutable std::mutex _completeMutex;
	mutable std::condition_variable _notifyComplete;
};

} // namespace cobalt::graphics
