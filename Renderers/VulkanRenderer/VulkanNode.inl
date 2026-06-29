// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
template<class T>
bool VulkanNode<T>::IsDrawStateCurrent() const
{
	return _drawStateCurrent;
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanNode<T>::FlagDrawStateNotCurrent()
{
	_drawStateCurrent = false;
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanNode<T>::FlagDrawStateCurrent()
{
	_drawStateCurrent = true;
}

} // namespace cobalt::graphics
