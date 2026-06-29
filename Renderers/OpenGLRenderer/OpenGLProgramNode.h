// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "OpenGLNode.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <unordered_set>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLShaderProgram;
class OpenGLStateGroupNode;

class OpenGLProgramNode : public OpenGLNode<IProgramNode>
{
public:
	// Constructors
	OpenGLProgramNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLProgramNode();

	// Initialization methods
	void Delete() override;

	// Child node methods
	void AddChildNode(IStateGroupNode* childNode) override;
	void AddChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) override;
	void AddChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveChildNode(IStateGroupNode* childNode) override;
	void RemoveChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) override;
	void RemoveChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveAllChildNodes() override;
	void SetChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount) override;
	void SetChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	const std::vector<OpenGLStateGroupNode*>& GetChildNodes() const;

	// Shader program methods
	SuccessToken BindShaderProgram(IShaderProgram* program) override;
	OpenGLShaderProgram* GetShaderProgram() const;

	// Build state methods
	void MigrateBuildStateToDrawState();
	const std::vector<IStateValueInfo*>& GetConstantValueEntries() const;

	// Constant state value methods
	void ResetConstantStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) override;

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

protected:
	// Constant state value methods
	void SetConstantStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetConstantStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;

private:
	// Structures
	struct MutableState
	{
		std::vector<OpenGLStateGroupNode*> childNodes;
		std::vector<IStateValueInfo*> valueEntries;
		std::vector<IStateValueInfo*> valueEntriesToDelete;
		std::string debugName = "Program";
	};

private:
	// Child node methods
	void RemoveChildNodes(const std::unordered_set<IStateGroupNode*>& childNodes);

	// Constant state value methods
	void SetConstantStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IStateValueInfo* value);

private:
	// Build state methods
	void DeleteRemovedEntries(MutableState& targetState);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState;
	MutableState _buildState;
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	OpenGLShaderProgram* _program;
};

} // namespace cobalt::graphics
