// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IStateGroupNode.h"
#include "MatrixTypes.h"
#include "Tokens.h"
#include "VectorTypes.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;
class IStateGroupNode;
class IShaderProgram;

class IProgramNode
{
public:
	// Typedefs
	typedef std::unique_ptr<IProgramNode, Deleter<IProgramNode>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Child node methods
	virtual void AddChildNode(IStateGroupNode* childNode) = 0;
	virtual void AddChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void AddChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveChildNode(IStateGroupNode* childNode) = 0;
	virtual void RemoveChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveAllChildNodes() = 0;
	virtual void SetChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void SetChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;

	// Shader program methods
	virtual SuccessToken BindShaderProgram(IShaderProgram* program) = 0;

	// Constant state value methods
	template<class T>
	inline void SetConstantStateValue(StateValueId stateId, const T& value);
	template<class T, class... IndexTs>
	inline void SetConstantStateValue(StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	template<class T>
	inline void SetConstantStateValue(StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline void ResetConstantStateValue(StateValueId stateId);
	template<class... IndexTs>
	inline void ResetConstantStateValue(StateValueId stateId, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	virtual void ResetConstantStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) = 0;

	// Debug methods
	virtual void SetDebugName(const Marshal::In<std::string>& name) = 0;

protected:
	// Constructors
	~IProgramNode() = default;

	// Constant state value methods
	virtual void SetConstantStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetConstantStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
};

}} // namespace cobalt::graphics
#include "IProgramNode.inl"
