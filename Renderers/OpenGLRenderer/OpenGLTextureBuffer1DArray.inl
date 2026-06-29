// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBuffer1DArray::OpenGLTextureBuffer1DArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureBuffer<ITextureBuffer1DArray, V1UInt32>(log, renderer, GL_TEXTURE_1D_ARRAY)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer1DArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer1DArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer1DArray::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
	// Generate the texture
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage2D(GL_TEXTURE_1D_ARRAY, mipmapLevels, internalFormat, (GLsizei)imageDimensions.X(), (GLsizei)arraySize);
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage2D(GL_TEXTURE_1D_ARRAY, entry.mipmapLevel, 0, (GLint)entry.arrayIndex, (GLsizei)mipmapDimensions.X(), 1, entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage2D(GL_TEXTURE_1D_ARRAY, entry.mipmapLevel, 0, (GLint)entry.arrayIndex, (GLsizei)mipmapDimensions.X(), 1, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
		}
	}
#else
	GLenum dummyNativeImageFormat;
	GLenum dummyNativeDataFormat;
	GetImageAllocationFormatNative(imageFormat, dataFormat, dummyNativeImageFormat, dummyNativeDataFormat);
	for (int i = 0; i < mipmapLevels; ++i)
	{
		auto mipmapDimensions = MipmapLevelDimensions(i);
		glTexImage2D(GL_TEXTURE_1D_ARRAY, i, internalFormat, (GLsizei)mipmapDimensions.X(), (GLsizei)arraySize, 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
	}
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage2D(GL_TEXTURE_1D_ARRAY, entry.mipmapLevel, 0, (GLint)entry.arrayIndex, (GLsizei)mipmapDimensions.X(), 1, entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage2D(GL_TEXTURE_1D_ARRAY, entry.mipmapLevel, 0, (GLint)entry.arrayIndex, (GLsizei)mipmapDimensions.X(), 1, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
		}
	}
	glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer1DArray::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = (pendingWrite.imageRegionInPixels.X() == 0) ? V1UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage2D(GL_TEXTURE_1D_ARRAY, pendingWrite.mipmapLevel, (GLint)pendingWrite.imageOffsetInPixels.X(), (GLint)pendingWrite.arrayIndex, (GLsizei)targetRegionDimensions.X(), 1, pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_1D_ARRAY, pendingWrite.mipmapLevel, (GLint)pendingWrite.imageOffsetInPixels.X(), (GLint)pendingWrite.arrayIndex, (GLsizei)targetRegionDimensions.X(), 1, pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	CheckGLError(Log());
	return true;
}

} // namespace cobalt::graphics
