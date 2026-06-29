// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureBuffer.h"
namespace cobalt::graphics {

class OpenGLTextureBufferCube : public OpenGLTextureBuffer<ITextureBufferCube, V2UInt32>
{
public:
	// Constructors
	inline OpenGLTextureBufferCube(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

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
#include "OpenGLTextureBufferCube.inl"
