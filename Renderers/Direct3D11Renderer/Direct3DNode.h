// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

template<class T>
class Direct3DNode : public T
{
public:
	// Constructors
	inline Direct3DNode() = default;

	// Build state methods
	inline bool IsDrawStateCurrent() const;

protected:
	// Build state methods
	inline void FlagDrawStateNotCurrent();
	inline void FlagDrawStateCurrent();

private:
	bool _drawStateCurrent = false;
};

} // namespace cobalt::graphics
#include "Direct3DNode.inl"
