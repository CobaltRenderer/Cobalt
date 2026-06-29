// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <iostream>
#include <memory>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
inline void CheckGLErrorFunction(const cobalt::logging::ILogger* log, const char* file, int32_t line)
{
	GLenum error = glGetError();
	if (error == GL_NO_ERROR)
	{
		return;
	}

	const char* errorString;
	switch (error)
	{
	case GL_INVALID_ENUM:
		errorString = "GL_INVALID_ENUM";
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
		break;
	case GL_INVALID_VALUE:
		errorString = "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		errorString = "GL_INVALID_OPERATION";
		break;
	case GL_OUT_OF_MEMORY:
		errorString = "GL_OUT_OF_MEMORY";
		break;
	default:
		errorString = "Unknown error";
		break;
	}
	log->Error("GL error: {0}, file {1}, line {2}: {3}", error, file, line, errorString);
}

//----------------------------------------------------------------------------------------
#ifdef _DEBUG
#define CheckGLError(log) CheckGLErrorFunction(log, __FILE__, __LINE__)
#else
#define CheckGLError(log)
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
inline void CheckEGLErrorFunction(const cobalt::logging::ILogger* log, const char* file, int32_t line)
{
	EGLint error = eglGetError();
	if (error == EGL_SUCCESS)
	{
		return;
	}

	const char* errorString;
	switch (error)
	{
	case EGL_NOT_INITIALIZED:
		errorString = "EGL_NOT_INITIALIZED";
		break;
	case EGL_BAD_ACCESS:
		errorString = "EGL_BAD_ACCESS";
		break;
	case EGL_BAD_ALLOC:
		errorString = "EGL_BAD_ALLOC";
		break;
	case EGL_BAD_ATTRIBUTE:
		errorString = "EGL_BAD_ATTRIBUTE";
		break;
	case EGL_BAD_CONTEXT:
		errorString = "EGL_BAD_CONTEXT";
		break;
	case EGL_BAD_CONFIG:
		errorString = "EGL_BAD_CONFIG";
		break;
	case EGL_BAD_CURRENT_SURFACE:
		errorString = "EGL_BAD_CURRENT_SURFACE";
		break;
	case EGL_BAD_DISPLAY:
		errorString = "EGL_BAD_DISPLAY";
		break;
	case EGL_BAD_SURFACE:
		errorString = "EGL_BAD_SURFACE";
		break;
	case EGL_BAD_MATCH:
		errorString = "EGL_BAD_MATCH";
		break;
	case EGL_BAD_PARAMETER:
		errorString = "EGL_BAD_PARAMETER";
		break;
	case EGL_BAD_NATIVE_PIXMAP:
		errorString = "EGL_BAD_NATIVE_PIXMAP";
		break;
	case EGL_BAD_NATIVE_WINDOW:
		errorString = "EGL_BAD_NATIVE_WINDOW";
		break;
	case EGL_CONTEXT_LOST:
		errorString = "EGL_CONTEXT_LOST";
		break;
	default:
		errorString = "Unknown error";
		break;
	}
	log->Error("EGL error: {0}, file {1}, line {2}: {3}", error, file, line, errorString);
}

//----------------------------------------------------------------------------------------
#ifdef _DEBUG
#define CheckEGLError(log) CheckEGLErrorFunction(log, __FILE__, __LINE__)
#else
#define CheckEGLError(log)
#endif
#endif

} // namespace cobalt::graphics
