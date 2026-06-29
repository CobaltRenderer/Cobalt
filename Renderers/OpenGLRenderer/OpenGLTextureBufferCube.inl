// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBufferCube::OpenGLTextureBufferCube(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureBuffer<ITextureBufferCube, V2UInt32>(log, renderer, GL_TEXTURE_CUBE_MAP)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBufferCube::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBufferCube::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBufferCube::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
	// Generate the texture
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmapLevels, internalFormat, imageDimensions.X(), imageDimensions.Y());
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + entry.arrayIndex), entry.mipmapLevel, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + entry.arrayIndex), entry.mipmapLevel, 0, 0, mipmapDimensions.X(), mipmapDimensions.Y(), entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
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
			for (int faceNo = 0; faceNo < 6; ++faceNo)
			{
				glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceNo), i, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
			}
		}
	}
	else
	{
		for (const auto& entry : initialData)
		{
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			if (isCompressedTexture)
			{
				glCompressedTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + entry.arrayIndex), entry.mipmapLevel, internalFormat, (GLsizei)mipmapDimensions.X(), (GLsizei)mipmapDimensions.Y(), 0, (GLsizei)entry.dataSizeInBytes, entry.data);
			}
			else
			{
				glTexImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + entry.arrayIndex), entry.mipmapLevel, internalFormat, (GLsizei)mipmapDimensions.X(), (GLsizei)mipmapDimensions.Y(), 0, entry.nativeImageFormat, entry.nativeDataFormat, entry.data);
			}
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBufferCube::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0)) ? V2UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), imageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + pendingWrite.arrayIndex), pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), targetRegionDimensions.X(), targetRegionDimensions.Y(), pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage2D(GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + pendingWrite.arrayIndex), pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), targetRegionDimensions.X(), targetRegionDimensions.Y(), pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	CheckGLError(Log());
	return true;
}

} // namespace cobalt::graphics
