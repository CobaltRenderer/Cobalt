// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace graphics {

template<class T>
class Deleter
{
public:
	inline void operator()(T* target)
	{
		target->Delete();
	}
};

}} // namespace cobalt::graphics
