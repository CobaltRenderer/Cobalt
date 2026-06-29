// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBuffer2D::OpenGLTextureBuffer2D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureBuffer<ITextureBuffer2D, V2UInt32>(log, renderer, GL_TEXTURE_2D)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer2D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer2D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer2D::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
	// If this is a multisample texture, we don't support initial data and have a different generation pathway. In that
	// case, we generate the texture here and return the result to the caller.
	auto sampleCount = GetSampleCount();
	if (sampleCount != SampleCount::SampleCount1)
	{
		auto imageDimensions = MipmapLevelDimensions(0);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, GetNativeSampleCountFromSampleCount(sampleCount), internalFormat, (GLsizei)imageDimensions.X(), (GLsizei)imageDimensions.Y(), GL_FALSE);
		CheckGLError(Log());
		return true;
	}

	// Generate the texture
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage2D(GL_TEXTURE_2D, mipmapLevels, internalFormat, imageDimensions.X(), imageDimensions.Y());
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage2D(GL_TEXTURE_2D, entry.mipmapLevel, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage2D(GL_TEXTURE_2D, entry.mipmapLevel, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
			if (!entry.convertedStencilData.empty())
			{
				glTexSubImage2D(GL_TEXTURE_2D, entry.mipmapLevel, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), entry.stencilNativeImageFormat, entry.stencilNativeDataFormat, reinterpret_cast<const void*>(entry.convertedStencilData.data()));
			}
		}
	}
#else
	if (initialData.empty())
	{
		GLenum dummyNativeImageFormat;
		GLenum dummyNativeDataFormat;
		GetImageAllocationFormatNative(imageFormat, dataFormat, dummyNativeImageFormat, dummyNativeDataFormat);
		for (int i = 0; i < mipmapLevels; ++i)
		{
			auto mipmapDimensions = MipmapLevelDimensions(i);
			glTexImage2D(GL_TEXTURE_2D, i, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
		}
	}
	else
	{
		for (const auto& entry : initialData)
		{
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			if (isCompressedTexture)
			{
				glCompressedTexImage2D(GL_TEXTURE_2D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), 0, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), 0, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
				if (!entry.convertedStencilData.empty())
				{
					glTexImage2D(GL_TEXTURE_2D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), 0, entry.stencilNativeImageFormat, entry.stencilNativeDataFormat, reinterpret_cast<const void*>(entry.convertedStencilData.data()));
				}
			}
		}
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer2D::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0)) ? V2UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), imageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage2D(GL_TEXTURE_2D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), targetRegionDimensions.X(), targetRegionDimensions.Y(), pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), targetRegionDimensions.X(), targetRegionDimensions.Y(), pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
		if (!pendingWrite.convertedStencilData.empty())
		{
			glTexSubImage2D(GL_TEXTURE_2D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), targetRegionDimensions.X(), targetRegionDimensions.Y(), pendingWrite.stencilNativeImageFormat, pendingWrite.stencilNativeDataFormat, reinterpret_cast<const void*>(pendingWrite.convertedStencilData.data()));
		}
	}
	CheckGLError(Log());
	return true;
}

} // namespace cobalt::graphics
