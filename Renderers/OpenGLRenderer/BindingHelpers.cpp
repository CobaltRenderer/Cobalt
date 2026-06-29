// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "BindingHelpers.h"
#include "OpenGLDataArray.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLShaderProgram.h"
#include "OpenGLStateBuffer.h"
#include "OpenGLTexelArray.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// StateBufferBindingInfo methods
//----------------------------------------------------------------------------------------
StateBufferBindingInfo::StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer)
: _stateBufferId(stateBufferId), _stateBufferPageNo(stateBufferPageNo), _stateBuffer(stateBuffer)
{}

//----------------------------------------------------------------------------------------
StateBufferId StateBufferBindingInfo::GetStateBufferId() const
{
	return _stateBufferId;
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::BindStateBuffer(OpenGLRenderer* renderer, OpenGLShaderProgram* program)
{
	// Retrieve the binding point if required
	if (!_retrievedBindingPoint)
	{
		program->GetBindPointForStateBuffer(_stateBufferId, _bindingPoint);
		_retrievedBindingPoint = true;
	}

	// Bind the state buffer
	auto* stateBufferResolved = KnownDynamicCast<OpenGLStateBuffer*>(_stateBuffer);
	stateBufferResolved->BindStateBufferPage(_bindingPoint, _stateBufferPageNo);
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::UnbindStateBuffer() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//----------------------------------------------------------------------------------------
// ResourceArrayBindingInfo methods
//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter)
#ifdef GL_VERSION_4_3
: _resourceArrayId(resourceArrayId), _dataArray(KnownDynamicCast<OpenGLDataArray*>(resourceBuffer)), _resetCounter(resetCounter), _isDataArrayBinding(true)
#else
: _resourceArrayId(resourceArrayId), _dataArray(nullptr), _resetCounter(resetCounter), _isDataArrayBinding(true)
#endif
{}

//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer)
#ifdef GL_VERSION_4_3
: _resourceArrayId(resourceArrayId), _texelArray(KnownDynamicCast<OpenGLTexelArray*>(resourceBuffer)), _isDataArrayBinding(false)
#else
: _resourceArrayId(resourceArrayId), _texelArray(nullptr), _isDataArrayBinding(false)
#endif
{}

//----------------------------------------------------------------------------------------
ResourceArrayId ResourceArrayBindingInfo::GetResourceArrayId() const
{
	return _resourceArrayId;
}

//----------------------------------------------------------------------------------------
void ResourceArrayBindingInfo::BindResourceArray(OpenGLRenderer* renderer, OpenGLShaderProgram* program, bool performReset)
{
#ifdef GL_VERSION_4_3
	// Retrieve the binding point if required
	if (!_retrievedBindingPoint)
	{
		program->GetBindPointForResourceArray(_resourceArrayId, _bindingPoint, _counterBindingPoint, _textureUnitNo, _writeableResource);
		_retrievedBindingPoint = true;
	}

	// Clear the counter if required
	if (performReset && _resetCounter && _isDataArrayBinding && _dataArray->HasCounter())
	{
		_dataArray->ResetCounter();
	}

	// Bind the resource array
	if (_isDataArrayBinding)
	{
		// Bind the data array
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _dataArray->GetBufferNo());
		if (_dataArray->HasCounter())
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _counterBindingPoint, _dataArray->GetCounterBufferNo());
		}

		// Record the fact that this resource array has been used at least once in this frame
		_dataArray->AddAsCurrentBuffer();
	}
	else
	{
		// Bind the texel array
		if (_writeableResource)
		{
			glBindImageTexture(_textureUnitNo, _texelArray->GetTextureNo(), 0, GL_TRUE, 0, GL_READ_WRITE, _texelArray->GetTextureFormat());
		}
		else
		{
			glActiveTexture(GL_TEXTURE0 + _textureUnitNo);
			glBindTexture(GL_TEXTURE_BUFFER, _texelArray->GetTextureNo());
		}

		// Record the fact that this resource array has been used at least once in this frame
		_texelArray->AddAsCurrentBuffer();
	}
#endif
}

//----------------------------------------------------------------------------------------
void ResourceArrayBindingInfo::UnbindResourceArray()
{
#ifdef GL_VERSION_4_3
	if (_isDataArrayBinding)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, 0);
		if (_dataArray->HasCounter())
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _counterBindingPoint, 0);
		}
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + _textureUnitNo);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}
#endif
}

} // namespace cobalt::graphics
