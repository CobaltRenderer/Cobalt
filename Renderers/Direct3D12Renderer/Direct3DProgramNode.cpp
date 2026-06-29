// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DProgramNode.h"
#include "Direct3DRenderableNode.h"
#include "Direct3DRenderer.h"
#include "Direct3DStateGroupNode.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <type_traits>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DProgramNode::Direct3DProgramNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _renderer(renderer), _log(log), _program(nullptr)
{}

//----------------------------------------------------------------------------------------
Direct3DProgramNode::~Direct3DProgramNode()
{
	DeleteRemovedEntries(_drawState);
	DeleteRemovedEntries(_buildState);
	for (auto& valueEntry : _buildState.valueEntries)
	{
		delete valueEntry;
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DProgramNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Direct3DProgramNode::AddChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto* stateGroupNode = KnownDynamicCast<Direct3DStateGroupNode*>(childNode);
	_buildState.childNodes.push_back(stateGroupNode);
	stateGroupNode->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());

	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::AddChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<Direct3DStateGroupNode*>(*(childNodes++));
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::AddChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<Direct3DStateGroupNode*>((childNodes++)->get());
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::RemoveChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (*i == childNode)
		{
			auto* stateGroupNode = KnownDynamicCast<Direct3DStateGroupNode*>(childNode);
			stateGroupNode->ClearAllFrameBufferEntries();

			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::RemoveChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::RemoveChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::RemoveChildNodes(const std::unordered_set<IStateGroupNode*>& childNodes)
{
	for (auto* child : childNodes)
	{
		KnownDynamicCast<Direct3DStateGroupNode*>(child)->ClearAllFrameBufferEntries();
	}

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
void Direct3DProgramNode::RemoveAllChildNodes()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);

	for (auto* child : _buildState.childNodes)
	{
		child->ClearAllFrameBufferEntries();
	}

	_buildState.childNodes.clear();
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);

	for (auto* child : _buildState.childNodes)
	{
		child->ClearAllFrameBufferEntries();
	}

	_buildState.childNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<Direct3DStateGroupNode*>(*(childNodes++));
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);

	for (auto* child : _buildState.childNodes)
	{
		child->ClearAllFrameBufferEntries();
	}

	_buildState.childNodes.resize(childNodeCount);

	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<Direct3DStateGroupNode*>((childNodes++)->get());
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}

	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<Direct3DStateGroupNode*>& Direct3DProgramNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Shader program methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DProgramNode::BindShaderProgram(IShaderProgram* program)
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
	_program = KnownDynamicCast<Direct3DShaderProgram*>(program);
	return true;
}

//----------------------------------------------------------------------------------------
Direct3DShaderProgram* Direct3DProgramNode::GetShaderProgram() const
{
	return _program;
}

//----------------------------------------------------------------------------------------
// Framebuffer association methods
//----------------------------------------------------------------------------------------
int Direct3DProgramNode::UpdateFrameBufferAssociation(int oldFrameBufferIndex, Direct3DFrameBuffer* newFrameBuffer)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (oldFrameBufferIndex >= 0)
	{
		// Remove the association between the render pass and the old framebuffer
		FrameBufferEntry& frameBufferEntry = _buildState.frameBufferEntries[oldFrameBufferIndex];
		int newUsageCount = --frameBufferEntry.usageCount;

		// If no associations remain to the framebuffer, remove the framebuffer entry from this node and all child state
		// group nodes.
		if (newUsageCount <= 0)
		{
			// Remove the framebuffer entry from all child state group nodes
			for (auto* childNode : _buildState.childNodes)
			{
				childNode->ClearFrameBufferEntry(oldFrameBufferIndex);
			}

			// Clear this framebuffer entry, and add the index number to our set of free framebuffer indices. Note that we
			// never erase entries from our framebuffer arrays once they are allocated, we simply mark them as free and re-use
			// them later if required. This allows us to avoid the expensive task of re-allocating indices for other
			// framebuffers that might have existed after the removed framebuffer, which would involve tracking and
			// communicating with all other render pass nodes that contain this program node.
			frameBufferEntry.frameBuffer = nullptr;
			_buildState.emptyFrameBufferIndices.push_back(oldFrameBufferIndex);
		}
	}

	int newFrameBufferIndex = -1;
	if (newFrameBuffer != nullptr)
	{
		// Attempt to locate an existing framebuffer entry for the specified framebuffer
		bool foundExistingFrameBufferEntry = false;
		size_t searchFrameBufferIndex = 0;
		while (searchFrameBufferIndex < _buildState.frameBufferEntries.size())
		{
			if (_buildState.frameBufferEntries[searchFrameBufferIndex].frameBuffer == newFrameBuffer)
			{
				foundExistingFrameBufferEntry = true;
				newFrameBufferIndex = (int)searchFrameBufferIndex;
				break;
			}
			++searchFrameBufferIndex;
		}

		// Update or create the framebuffer entry as required
		if (foundExistingFrameBufferEntry)
		{
			// Increment the usage count for the new reference to this framebuffer entry
			++_buildState.frameBufferEntries[newFrameBufferIndex].usageCount;
		}
		else
		{
			// Determine the index to use for the new framebuffer entry
			if (!_buildState.emptyFrameBufferIndices.empty())
			{
				newFrameBufferIndex = _buildState.emptyFrameBufferIndices.back();
				_buildState.emptyFrameBufferIndices.pop_back();
			}
			else
			{
				newFrameBufferIndex = (int)_buildState.frameBufferEntries.size();
				_buildState.frameBufferEntries.resize(newFrameBufferIndex + 1);
			}

			// Create the new framebuffer entry, and set the initial usage count to 1.
			FrameBufferEntry& frameBufferEntry = _buildState.frameBufferEntries[newFrameBufferIndex];
			frameBufferEntry.usageCount = 1;
			frameBufferEntry.frameBuffer = newFrameBuffer;

			// Add this new framebuffer entry to all child state group nodes
			for (auto* childNode : _buildState.childNodes)
			{
				childNode->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
			}
		}
	}
	return newFrameBufferIndex;
}

//----------------------------------------------------------------------------------------
// Constant state value methods
//----------------------------------------------------------------------------------------
void Direct3DProgramNode::ResetConstantStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount)
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
	for (size_t i = 0; i < _buildState.valueEntries.size(); ++i)
	{
		IConstantStateValueInfo* entry = _buildState.valueEntries[i];
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
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetConstantStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IConstantStateValueInfo* value)
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
		IConstantStateValueInfo* entry = valueEntry;
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
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DProgramNode::MigrateBuildStateToDrawState()
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
	for (Direct3DStateGroupNode* entry : _drawState.childNodes)
	{
		entry->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DProgramNode::DeleteRemovedEntries(MutableState& targetState)
{
	for (auto& i : targetState.valueEntriesToDelete)
	{
		delete i;
	}
	targetState.valueEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
const std::vector<IConstantStateValueInfo*>& Direct3DProgramNode::GetConstantValueEntries() const
{
	return _drawState.valueEntries;
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Direct3DProgramNode::SetDebugName(const Marshal::In<std::string>& name)
{
	// String is stored as wide string because render markers require a wide string
	_buildState.debugName = UTF8ToUTF16(name);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::wstring& Direct3DProgramNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
