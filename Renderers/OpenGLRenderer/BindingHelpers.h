// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include "OpenGLShaderProgram.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLDataArray;
class OpenGLTexelArray;

//----------------------------------------------------------------------------------------
class IStateValueInfo
{
public:
	inline virtual ~IStateValueInfo() = 0;
	virtual StateValueId GetAttributeId() const = 0;
	virtual size_t GetArrayIndexCount() const = 0;
	virtual const size_t* GetArrayIndices() const = 0;
	virtual void ApplyValue(OpenGLShaderProgram* program) const = 0;
};

//----------------------------------------------------------------------------------------
class ITextureBindingInfo
{
public:
	inline virtual ~ITextureBindingInfo() = 0;
	virtual TextureId GetTextureId() const = 0;
	virtual void BindTexture() const = 0;
	virtual void UnbindTexture() const = 0;
};

//----------------------------------------------------------------------------------------
class StateBufferBindingInfo
{
public:
	StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer);
	StateBufferId GetStateBufferId() const;
	void BindStateBuffer(OpenGLRenderer* renderer, OpenGLShaderProgram* program);
	void UnbindStateBuffer() const;

private:
	IStateBuffer* _stateBuffer;
	StateBufferId _stateBufferId;
	uint32_t _stateBufferPageNo;
	GLuint _bindingPoint = 0;
	bool _retrievedBindingPoint = false;
};

//----------------------------------------------------------------------------------------
class ResourceArrayBindingInfo
{
public:
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter);
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer);
	ResourceArrayId GetResourceArrayId() const;
	void BindResourceArray(OpenGLRenderer* renderer, OpenGLShaderProgram* program, bool performReset);
	void UnbindResourceArray();

private:
	OpenGLDataArray* _dataArray = nullptr;
	OpenGLTexelArray* _texelArray = nullptr;
	ResourceArrayId _resourceArrayId;
	GLuint _bindingPoint = 0;
	GLuint _counterBindingPoint = 0;
	int _textureUnitNo = 0;
	bool _writeableResource = false;
	bool _isDataArrayBinding;
	bool _resetCounter = false;
	bool _retrievedBindingPoint = false;
};

//----------------------------------------------------------------------------------------
template<class ValueType>
class ConstantStateValueInfo : public IStateValueInfo
{
public:
	inline ConstantStateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline StateValueId GetAttributeId() const override;
	inline size_t GetArrayIndexCount() const override;
	inline const size_t* GetArrayIndices() const override;
	inline void ApplyValue(OpenGLShaderProgram* program) const override;

private:
	StateValueId _stateId;
	std::vector<size_t> _arrayIndices;
	ValueType _value;
};

//----------------------------------------------------------------------------------------
template<class ValueType>
class StateValueInfo : public IStateValueInfo
{
public:
	inline StateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline StateValueId GetAttributeId() const override;
	inline size_t GetArrayIndexCount() const override;
	inline const size_t* GetArrayIndices() const override;
	inline void ApplyValue(OpenGLShaderProgram* program) const override;

private:
	StateValueId _stateId;
	std::vector<size_t> _arrayIndices;
	ValueType _value;
};

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
class TextureBindingWithCombinedSamplerInfo : public ITextureBindingInfo
{
public:
	TextureBindingWithCombinedSamplerInfo(TextureId textureId, GLenum textureTarget, TextureType* texture, SamplerType* sampler);
	TextureId GetTextureId() const override;
	void BindTexture() const override;
	void UnbindTexture() const override;

private:
	TextureType* _texture;
	SamplerType* _sampler;
	TextureId _textureId;
	GLenum _textureTarget;
};

} // namespace cobalt::graphics
#include "BindingHelpers.inl"
