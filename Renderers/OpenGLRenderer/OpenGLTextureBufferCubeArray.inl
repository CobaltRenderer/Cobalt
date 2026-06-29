// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureBufferCubeArray::OpenGLTextureBufferCubeArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
#ifdef GL_VERSION_4_0
: OpenGLTextureBuffer<ITextureBufferCubeArray, V2UInt32>(log, renderer, GL_TEXTURE_CUBE_MAP_ARRAY)
#else
: OpenGLTextureBuffer<ITextureBufferCubeArray, V2UInt32>(log, renderer, 0)
#endif
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBufferCubeArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureBufferCubeArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
bool OpenGLTextureBufferCubeArray::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData)
{
#ifdef GL_VERSION_4_0
	// Generate the texture
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	bool isCompressedTexture = IsCompressedTexture();
#ifdef GL_VERSION_4_3
	auto imageDimensions = MipmapLevelDimensions(0);
	glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipmapLevels, internalFormat, imageDimensions.X(), imageDimensions.Y(), GLsizei(arraySize));
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, entry.mipmapLevel, 0, 0, (GLint)entry.arrayIndex, mipmapDimensions.X(), mipmapDimensions.Y(), 1, entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, entry.mipmapLevel, 0, 0, (GLint)entry.arrayIndex, mipmapDimensions.X(), mipmapDimensions.Y(), 1, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
		}
	}
#else
	GLenum dummyNativeImageFormat;
	GLenum dummyNativeDataFormat;
	GetImageAllocationFormatNative(imageFormat, dataFormat, dummyNativeImageFormat, dummyNativeDataFormat);
	for (int i = 0; i < mipmapLevels; ++i)
	{
		auto mipmapDimensions = MipmapLevelDimensions(i);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, i, internalFormat, mipmapDimensions.X(), mipmapDimensions.Y(), arraySize, 0, dummyNativeImageFormat, dummyNativeDataFormat, nullptr);
	}
	for (const auto& entry : initialData)
	{
		auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
		if (isCompressedTexture)
		{
			glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, entry.mipmapLevel, 0, 0, entry.arrayIndex, mipmapDimensions.X(), mipmapDimensions.Y(), 1, entry.nativeImageFormat, (GLsizei)entry.dataSizeInBytes, reinterpret_cast<const void*>(entry.data));
		}
		else
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, entry.mipmapLevel, 0, 0, entry.arrayIndex, mipmapDimensions.X(), mipmapDimensions.Y(), 1, entry.nativeImageFormat, entry.nativeDataFormat, reinterpret_cast<const void*>(entry.data));
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, mipmapLevels - 1);
#endif
	CheckGLError(Log());
	return true;
#else
	Log()->Error("Cubemap array textures are only supported on OpenGL 4.0+.");
	return false;
#endif
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool OpenGLTextureBufferCubeArray::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
#ifdef GL_VERSION_4_0
	// Update the texture data
	auto imageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0)) ? V2UInt32(imageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), imageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y()) : pendingWrite.imageRegionInPixels;
	if (IsCompressedTexture())
	{
		glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, pendingWrite.mipmapLevel, pendingWrite.imageOffsetInPixels.X(), pendingWrite.imageOffsetInPixels.Y(), (GLint)pendingWrite.arrayIndex, targetRegionDimensions.X(), targetRegionDimensions.Y(), 1, pendingWrite.nativeImageFormat, (GLsizei)pendingWrite.data.size(), reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	else
	{
		glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, pendingWrite.mipmapLevel, (GLint)pendingWrite.imageOffsetInPixels.X(), (GLint)pendingWrite.imageOffsetInPixels.Y(), (GLint)pendingWrite.arrayIndex, targetRegionDimensions.X(), targetRegionDimensions.Y(), 1, pendingWrite.nativeImageFormat, pendingWrite.nativeDataFormat, reinterpret_cast<const void*>(pendingWrite.data.data()));
	}
	CheckGLError(Log());
	return true;
#else
	return false;
#endif
}

} // namespace cobalt::graphics
