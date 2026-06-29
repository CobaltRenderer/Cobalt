// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBuffer1D::OpenGLTextureBuffer1D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureBuffer<ITextureBuffer1D, V1UInt32>(log, renderer, GL_TEXTURE_1D)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer1D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer1D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer1D::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
	// Generate the texture
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage1D(GL_TEXTURE_1D, mipmapLevels, internalFormat, imageDimensions.X());
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage1D(GL_TEXTURE_1D, entry.mipmapLevel, 0, mipmapDimensions.X(), entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage1D(GL_TEXTURE_1D, entry.mipmapLevel, 0, mipmapDimensions.X(), entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
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
			glTexImage1D(GL_TEXTURE_1D, i, internalFormat, mipmapDimensions.X(), 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
		}
	}
	else
	{
		for (const auto& entry : initialData)
		{
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			if (isCompressedTexture)
			{
				glCompressedTexImage1D(GL_TEXTURE_1D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), 0, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
			}
			else
			{
				glTexImage1D(GL_TEXTURE_1D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), 0, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
			}
		}
	}
	// Note that this MUST be set after we have called glTexImage* above for each mipmap level, or we see crashes on
	// older Intel integrated graphics devices, even with the latest drivers (as of 2026-05-19). This is a driver bug,
	// as it's perfectly legal for the texture to be incomplete at this stage prior to use, however the workaround is
	// simple - we simply define each mipmap level before adjusting GL_TEXTURE_MAX_LEVEL. This bug has been observed on
	// Intel HD Graphics 620 and Intel UHD Graphics 630 devices. Other devices are likely affected. Modern Intel
	// graphics devices do not show this issue. The issue also does not appear on any other texture types, including
	// GL_TEXTURE_1D_ARRAY, so it's a narrow bug limited to GL_TEXTURE_1D textures. We set GL_TEXTURE_MAX_LEVEL after
	// defining each mipmap level on all texture types for consistency and as a defensive measure though.
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer1D::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = (pendingWrite.imageRegionInPixels.X() == 0) ? V1UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage1D(GL_TEXTURE_1D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), targetRegionDimensions.X(), pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage1D(GL_TEXTURE_1D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), targetRegionDimensions.X(), pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	CheckGLError(Log());
	return true;
}

} // namespace cobalt::graphics
