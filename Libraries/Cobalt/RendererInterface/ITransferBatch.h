// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include <memory>
namespace cobalt { namespace graphics {

class ITransferBatch
{
public:
	// Enumerations
	enum class StartTiming
	{
		AfterCurrentFrame,
		Immediately,
	};
	enum class EndTiming
	{
		BeforeNextFrame,
		AnyFrame,
	};

	// Typedefs
	typedef std::unique_ptr<ITransferBatch, Deleter<ITransferBatch>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Submission methods
	virtual SuccessToken SubmitBatch() = 0;
	virtual bool IsSubmitted() const = 0;
	virtual bool IsComplete() const = 0;
	virtual void WaitForComplete() const = 0;

protected:
	// Constructors
	~ITransferBatch() = default;
};

}} // namespace cobalt::graphics
