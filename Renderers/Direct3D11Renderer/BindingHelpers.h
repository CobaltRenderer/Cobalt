// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include "Direct3DShaderProgram.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
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
	virtual void BindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) = 0;
	virtual void UnbindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) = 0;
};

//----------------------------------------------------------------------------------------
class ISamplerBindingInfo
{
public:
	inline virtual ~ISamplerBindingInfo() = 0;
	virtual SamplerId GetSamplerId() const = 0;
	virtual void BindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) = 0;
	virtual void UnbindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) = 0;
};

//----------------------------------------------------------------------------------------
class StateBufferBindingInfo
{
public:
	StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer);
	StateBufferId GetStateBufferId() const;
	void BindStateBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void UnbindStateBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);

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
	void BindResourceArray(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool performReset, std::vector<ID3D11UnorderedAccessView*>& resourceBufferViews, std::vector<UINT>& resetValues, UINT& lowestBindPoint);

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
	inline void BindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;
	inline void UnbindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;

private:
	ID3D11ShaderResourceView* _resourceView = nullptr;
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
	inline void BindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;
	inline void UnbindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;

private:
	ID3D11ShaderResourceView* _resourceView = nullptr;
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
	inline void BindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;
	inline void UnbindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program) override;

private:
	T* _sampler;
	const Direct3DShaderProgram::SamplerBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	SamplerId _samplerId;
	bool _retrievedBindPoints = false;
};

} // namespace cobalt::graphics
#include "BindingHelpers.inl"
