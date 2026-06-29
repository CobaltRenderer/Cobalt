// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLProgramNode.h"
#include "OpenGLRenderableNode.h"
#include "OpenGLRenderer.h"
#include "OpenGLStateGroupNode.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <type_traits>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLProgramNode::OpenGLProgramNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _renderer(renderer), _log(log), _program(nullptr)
{}

//----------------------------------------------------------------------------------------
OpenGLProgramNode::~OpenGLProgramNode()
{
	DeleteRemovedEntries(_drawState);
	DeleteRemovedEntries(_buildState);
	for (auto& valueEntrie : _buildState.valueEntries)
	{
		delete valueEntrie;
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLProgramNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void OpenGLProgramNode::AddChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.push_back(KnownDynamicCast<OpenGLStateGroupNode*>(childNode));
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::AddChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<OpenGLStateGroupNode*>(*(childNodes++));
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::AddChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<OpenGLStateGroupNode*>((childNodes++)->get());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::RemoveChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (*i == childNode)
		{
			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::RemoveChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::RemoveChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::RemoveChildNodes(const std::unordered_set<IStateGroupNode*>& childNodes)
{
	// Skip leading entries in the child array until we find the first entry to remove. This helps improve performance
	// when the elements to remove are towards the end of the child array.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t readIndex = 0;
	while ((readIndex < currentChildCount) && childNodes.find(_buildState.childNodes[readIndex]) == childNodes.end())
	{
		++readIndex;
	}

	// Start copying remaining entries in-place in the array, shuffling all the remaining entries forward, until we
	// reach the end of the array or remove all the target entries. The early abort when we've removed all target
	// entries helps improve performance when the elements to remove are towards the start of the child array.
	size_t writeIndex = readIndex++;
	size_t childCountToRemove = childNodes.size();
	size_t removedChildNodeCount = 1; // Start at 1 because we skip over the entry we identified in the loop above
	while (readIndex < currentChildCount)
	{
		if (childNodes.find(_buildState.childNodes[readIndex]) == childNodes.end())
		{
			_buildState.childNodes[writeIndex++] = _buildState.childNodes[readIndex];
		}
		else
		{
			if (++removedChildNodeCount == childCountToRemove)
			{
				++readIndex;
				break;
			}
		}
		++readIndex;
	}

	// Shuffle forward any remaining entries in the array now that we've removed all the target entries
	while (readIndex < currentChildCount)
	{
		_buildState.childNodes[writeIndex++] = _buildState.childNodes[readIndex++];
	}

	// Resize the array to trim off the dead entries at the end
	_buildState.childNodes.resize(writeIndex);

	// Flag the draw state as no longer being current
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::RemoveAllChildNodes()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.clear();
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<OpenGLStateGroupNode*>(*(childNodes++));
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<OpenGLStateGroupNode*>((childNodes++)->get());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<OpenGLStateGroupNode*>& OpenGLProgramNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Shader program methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLProgramNode::BindShaderProgram(IShaderProgram* program)
{
	// Ensure a shader program has been supplied
	if (program == nullptr)
	{
		_log->Error("Attempted to bind a null shader program to a program node");
		return false;
	}

	// Ensure the shader program has not already been set for this node
	if (_program != nullptr)
	{
		_log->Error("Attempted to bind a shader program to a program node which has already had a shader program bound");
		return false;
	}

	// Bind the shader program, and return true to the caller.
	_program = KnownDynamicCast<OpenGLShaderProgram*>(program);
	return true;
}

//----------------------------------------------------------------------------------------
OpenGLShaderProgram* OpenGLProgramNode::GetShaderProgram() const
{
	return _program;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLProgramNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		DeleteRemovedEntries(_drawState);
		_drawState = _buildState;
		_buildState.valueEntriesToDelete.clear();
		FlagDrawStateCurrent();
	}

	// Migrate build state in our child nodes
	for (OpenGLStateGroupNode* entry : _drawState.childNodes)
	{
		entry->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::DeleteRemovedEntries(MutableState& targetState)
{
	for (auto& i : targetState.valueEntriesToDelete)
	{
		delete i;
	}
	targetState.valueEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
const std::vector<IStateValueInfo*>& OpenGLProgramNode::GetConstantValueEntries() const
{
	return _drawState.valueEntries;
}

//----------------------------------------------------------------------------------------
// Constant state value methods
//----------------------------------------------------------------------------------------
void OpenGLProgramNode::ResetConstantStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to reset constant state value with ID \"{0}\"", stateId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	FlagDrawStateNotCurrent();

	// Remove any existing value that may have been set for the target state ID
	for (uint32_t i = 0; i < _buildState.valueEntries.size(); ++i)
	{
		IStateValueInfo* entry = _buildState.valueEntries[i];
		if ((entry->GetAttributeId() == stateId) && (entry->GetArrayIndexCount() == arrayIndexCount))
		{
			bool arrayIndicesMatch = true;
			auto entryArrayIndices = entry->GetArrayIndices();
			size_t arrayIndex = 0;
			while (arrayIndex < entry->GetArrayIndexCount())
			{
				if (entryArrayIndices[arrayIndex] != arrayIndices[arrayIndex])
				{
					arrayIndicesMatch = false;
					break;
				}
				++arrayIndex;
			}
			if (arrayIndicesMatch)
			{
				_buildState.valueEntriesToDelete.push_back(entry);
				_buildState.valueEntries.erase(_buildState.valueEntries.begin() + i);
				return;
			}
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetConstantStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IStateValueInfo* value)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to set state value with ID \"{0}\"", stateId);
		delete value;
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	FlagDrawStateNotCurrent();

	// Add a value entry for this state, removing any existing value that may already have been set for the target state
	// ID.
	for (auto& valueEntry : _buildState.valueEntries)
	{
		IStateValueInfo* entry = valueEntry;
		if ((entry->GetAttributeId() == stateId) && (entry->GetArrayIndexCount() == arrayIndexCount))
		{
			bool arrayIndicesMatch = true;
			auto entryArrayIndices = entry->GetArrayIndices();
			size_t arrayIndex = 0;
			while (arrayIndex < entry->GetArrayIndexCount())
			{
				if (entryArrayIndices[arrayIndex] != arrayIndices[arrayIndex])
				{
					arrayIndicesMatch = false;
					break;
				}
				++arrayIndex;
			}
			if (arrayIndicesMatch)
			{
				_buildState.valueEntriesToDelete.push_back(entry);
				valueEntry = value;
				return;
			}
		}
	}
	_buildState.valueEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void OpenGLProgramNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& OpenGLProgramNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
