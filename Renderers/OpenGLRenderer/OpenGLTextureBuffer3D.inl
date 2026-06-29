// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBuffer3D::OpenGLTextureBuffer3D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureBuffer<ITextureBuffer3D, V3UInt32>(log, renderer, GL_TEXTURE_3D)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer3D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBuffer3D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer3D::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
	// Generate the texture
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage3D(GL_TEXTURE_3D, mipmapLevels, internalFormat, imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z());
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage3D(GL_TEXTURE_3D, entry.mipmapLevel, 0, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), mipmapDimensions.Z(), entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage3D(GL_TEXTURE_3D, entry.mipmapLevel, 0, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), mipmapDimensions.Z(), entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
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
			glTexImage3D(GL_TEXTURE_3D, i, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), mipmapDimensions.Z(), 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
		}
	}
	else
	{
		for (const auto& entry : initialData)
		{
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			if (isCompressedTexture)
			{
				glCompressedTexImage3D(GL_TEXTURE_3D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), mipmapDimensions.Z(), 0, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
			}
			else
			{
				glTexImage3D(GL_TEXTURE_3D, entry.mipmapLevel, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), mipmapDimensions.Z(), 0, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
			}
		}
	}
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBuffer3D::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0) || (pendingWrite.imageRegionInPixels.Z() == 0)) ? V3UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), imageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y(), imageDimensions.Z() - pendingWrite.imageOffsetInPixels.Z()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage3D(GL_TEXTURE_3D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), pendingWrite.imageOffsetInPixels.Z(), targetRegionDimensions.X(), targetRegionDimensions.Y(), targetRegionDimensions.Z(), pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage3D(GL_TEXTURE_3D, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), pendingWrite.imageOffsetInPixels.Z(), targetRegionDimensions.X(), targetRegionDimensions.Y(), targetRegionDimensions.Z(), pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	CheckGLError(Log());
	return true;
}

} // namespace cobalt::graphics
