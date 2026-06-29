// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DescriptorHandle.h"
#include "Direct3DHeaders.h"
#include "Direct3DShaderProgram.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DDataArray;
class Direct3DTexelArray;

//----------------------------------------------------------------------------------------
class IConstantStateValueInfo
{
public:
	inline virtual ~IConstantStateValueInfo() = 0;
	virtual StateValueId GetAttributeId() const = 0;
	virtual size_t GetArrayIndexCount() const = 0;
	virtual const size_t* GetArrayIndices() const = 0;
	virtual void ApplyValue(Direct3DShaderProgram* program) const = 0;
};

//----------------------------------------------------------------------------------------
class IStateValueInfo
{
public:
	inline virtual ~IStateValueInfo() = 0;
	virtual StateValueId GetAttributeId() const = 0;
	virtual size_t GetArrayIndexCount() const = 0;
	virtual const size_t* GetArrayIndices() const = 0;
	virtual void ApplyValue(Direct3DShaderProgram* program) const = 0;
};

//----------------------------------------------------------------------------------------
class ITextureBindingInfo
{
public:
	inline virtual ~ITextureBindingInfo() = 0;
	virtual TextureId GetTextureId() const = 0;
	virtual void BindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) = 0;
	virtual void UnbindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) = 0;
};

//----------------------------------------------------------------------------------------
class ISamplerBindingInfo
{
public:
	inline virtual ~ISamplerBindingInfo() = 0;
	virtual SamplerId GetSamplerId() const = 0;
	virtual void BindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) = 0;
	virtual void UnbindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) = 0;
};

//----------------------------------------------------------------------------------------
class StateBufferBindingInfo
{
public:
	StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer);
	StateBufferId GetStateBufferId() const;
	void BindStateBuffer(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding);
	void UnbindStateBuffer(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding);

private:
	IStateBuffer* _stateBuffer;
	StateBufferId _stateBufferId;
	uint32_t _stateBufferPageNo;
};

//----------------------------------------------------------------------------------------
class ResourceArrayBindingInfo
{
public:
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter);
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer);
	ResourceArrayId GetResourceArrayId() const;
	void BindResourceArray(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool performReset, bool computeShaderBinding);
	void UnbindResourceArray(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding);
	bool HasCaptureTargets() const;

private:
	Direct3DDataArray* _dataArray = nullptr;
	Direct3DTexelArray* _texelArray = nullptr;
	const Direct3DShaderProgram::ResourceArrayBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	ResourceArrayId _resourceArrayId;
	bool _isDataArrayBinding;
	bool _resetCounter = false;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
template<class ValueType>
class ConstantStateValueInfo : public IConstantStateValueInfo
{
public:
	inline ConstantStateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline StateValueId GetAttributeId() const override;
	inline size_t GetArrayIndexCount() const override;
	inline const size_t* GetArrayIndices() const override;
	inline void ApplyValue(Direct3DShaderProgram* program) const override;

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
	inline void ApplyValue(Direct3DShaderProgram* program) const override;

private:
	StateValueId _stateId;
	std::vector<size_t> _arrayIndices;
	ValueType _value;
};

//----------------------------------------------------------------------------------------
template<class T>
class TextureBindingInfo : public ITextureBindingInfo
{
public:
	inline TextureBindingInfo(TextureId textureId, T* texture);
	inline TextureId GetTextureId() const override;
	inline void BindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;
	inline void UnbindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;

private:
	T* _texture;
	const Direct3DShaderProgram::TextureBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	TextureId _textureId;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
class TextureBindingWithCombinedSamplerInfo : public ITextureBindingInfo
{
public:
	inline TextureBindingWithCombinedSamplerInfo(TextureId textureId, TextureType* texture, SamplerType* sampler);
	inline TextureId GetTextureId() const override;
	inline void BindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;
	inline void UnbindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;

private:
	TextureType* _texture;
	SamplerType* _sampler;
	const Direct3DShaderProgram::TextureBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	TextureId _textureId;
	bool _retrievedBindingPoints = false;
};

//----------------------------------------------------------------------------------------
template<class T>
class SamplerBindingInfo : public ISamplerBindingInfo
{
public:
	inline SamplerBindingInfo(SamplerId samplerId, T* sampler);
	inline SamplerId GetSamplerId() const override;
	inline void BindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;
	inline void UnbindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding) override;

private:
	T* _sampler;
	const Direct3DShaderProgram::SamplerBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	SamplerId _samplerId;
	bool _retrievedBindPoints = false;
};

} // namespace cobalt::graphics
#include "BindingHelpers.inl"
