// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureBuffer.h"
namespace cobalt::graphics {

class OpenGLTextureBuffer1D : public OpenGLTextureBuffer<ITextureBuffer1D, V1UInt32>
{
public:
	// Constructors
	inline OpenGLTextureBuffer1D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

protected:
	// Build state methods
	inline void FlagObjectModified() final;
	inline bool CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData) final;

	// Data update methods
	inline bool CompletePendingDataWrite(const PendingWrite& pendingWrite) final;
};

} // namespace cobalt::graphics
#include "OpenGLTextureBuffer1D.inl"
