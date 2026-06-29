// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanProgramNode.h"
#include "VulkanRenderer.h"
#include "VulkanStateGroupNode.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanProgramNode::VulkanProgramNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _renderer(renderer), _log(log), _program(nullptr)
{}

//----------------------------------------------------------------------------------------
VulkanProgramNode::~VulkanProgramNode()
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
void VulkanProgramNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void VulkanProgramNode::AddChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto* stateGroupNode = KnownDynamicCast<VulkanStateGroupNode*>(childNode);
	_buildState.childNodes.push_back(stateGroupNode);
	stateGroupNode->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::AddChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<VulkanStateGroupNode*>(*(childNodes++));
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::AddChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<VulkanStateGroupNode*>((childNodes++)->get());
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::RemoveChildNode(IStateGroupNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (*i == childNode)
		{
			auto* stateGroupNode = KnownDynamicCast<VulkanStateGroupNode*>(childNode);
			stateGroupNode->ClearAllFrameBufferEntries();

			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::RemoveChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::RemoveChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IStateGroupNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::RemoveChildNodes(const std::unordered_set<IStateGroupNode*>& childNodes)
{
	for (auto* child : childNodes)
	{
		KnownDynamicCast<VulkanStateGroupNode*>(child)->ClearAllFrameBufferEntries();
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
void VulkanProgramNode::RemoveAllChildNodes()
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
void VulkanProgramNode::SetChildNodes(IStateGroupNode* const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);

	for (auto* child : _buildState.childNodes)
	{
		child->ClearAllFrameBufferEntries();
	}

	_buildState.childNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<VulkanStateGroupNode*>(*(childNodes++));
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetChildNodes(IStateGroupNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);

	for (auto* child : _buildState.childNodes)
	{
		child->ClearAllFrameBufferEntries();
	}

	_buildState.childNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		_buildState.childNodes[i] = KnownDynamicCast<VulkanStateGroupNode*>((childNodes++)->get());
		_buildState.childNodes[i]->SetFrameBufferCount((int)_buildState.frameBufferEntries.size());
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<VulkanStateGroupNode*>& VulkanProgramNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Shader program methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanProgramNode::BindShaderProgram(IShaderProgram* program)
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
	_program = KnownDynamicCast<VulkanShaderProgram*>(program);
	return true;
}

//----------------------------------------------------------------------------------------
VulkanShaderProgram* VulkanProgramNode::GetShaderProgram() const
{
	return _program;
}

//----------------------------------------------------------------------------------------
// Framebuffer association methods
//----------------------------------------------------------------------------------------
int VulkanProgramNode::UpdateFrameBufferAssociation(int oldFrameBufferIndex, VulkanFrameBuffer* newFrameBuffer)
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
int VulkanProgramNode::UpdateDefaultStateAssociation(int oldDefaultStateIndex, VulkanDefaultState* newDefaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (oldDefaultStateIndex >= 0)
	{
		// Remove the association between the render pass and the old default state entry
		DefaultStateEntry& defaultStateEntry = _buildState.defaultStateEntries[oldDefaultStateIndex];
		int newUsageCount = --defaultStateEntry.usageCount;

		// If no associations remain to the default state, remove the default state entry from this node and all child state
		// group nodes.
		if (newUsageCount <= 0)
		{
			// Clear this default state entry, and add the index number to our set of free indices. Note that we never erase
			// entries from our arrays once they are allocated, we simply mark them as free and re-use them later if required.
			// This allows us to avoid the expensive task of re-allocating indices for other default state entries that might
			// have existed after the removed default state, which would involve tracking and communicating with all other
			// render pass nodes that contain this program node.
			defaultStateEntry.defaultState = nullptr;
			_buildState.emptyDefaultStateIndices.push_back(oldDefaultStateIndex);
		}
	}

	int newDefaultStateIndex = -1;
	if (newDefaultState != nullptr)
	{
		// Attempt to locate an existing default state entry for the specified default state
		bool foundExistingDefaultStateEntry = false;
		size_t searchDefaultStateIndex = 0;
		while (searchDefaultStateIndex < _buildState.defaultStateEntries.size())
		{
			if (_buildState.defaultStateEntries[searchDefaultStateIndex].defaultState == newDefaultState)
			{
				foundExistingDefaultStateEntry = true;
				newDefaultStateIndex = (int)searchDefaultStateIndex;
				break;
			}
			++searchDefaultStateIndex;
		}

		// Update or create the default state entry as required
		if (foundExistingDefaultStateEntry)
		{
			// Increment the usage count for the new reference to this default state entry
			++_buildState.defaultStateEntries[newDefaultStateIndex].usageCount;
		}
		else
		{
			// Determine the index to use for the new default state entry
			if (!_buildState.emptyDefaultStateIndices.empty())
			{
				newDefaultStateIndex = _buildState.emptyDefaultStateIndices.back();
				_buildState.emptyDefaultStateIndices.pop_back();
			}
			else
			{
				newDefaultStateIndex = (int)_buildState.defaultStateEntries.size();
				_buildState.defaultStateEntries.resize(newDefaultStateIndex + 1);
			}

			// Create the new default state entry, and set the initial usage count to 1.
			DefaultStateEntry& defaultStateEntry = _buildState.defaultStateEntries[newDefaultStateIndex];
			defaultStateEntry.usageCount = 1;
			defaultStateEntry.defaultState = newDefaultState;
		}
	}
	return newDefaultStateIndex;
}

//----------------------------------------------------------------------------------------
// Constant state value methods
//----------------------------------------------------------------------------------------
void VulkanProgramNode::ResetConstantStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount)
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
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//---------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, arrayIndices, arrayIndexCount, new ConstantStateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetConstantStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IConstantStateValueInfo* value)
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
void VulkanProgramNode::MigrateBuildStateToDrawState()
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
	for (VulkanStateGroupNode* entry : _drawState.childNodes)
	{
		entry->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
const std::vector<IConstantStateValueInfo*>& VulkanProgramNode::GetConstantValueEntries() const
{
	return _drawState.valueEntries;
}

//----------------------------------------------------------------------------------------
void VulkanProgramNode::DeleteRemovedEntries(MutableState& targetState)
{
	for (auto& entry : targetState.valueEntriesToDelete)
	{
		delete entry;
	}
	targetState.valueEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void VulkanProgramNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& VulkanProgramNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
