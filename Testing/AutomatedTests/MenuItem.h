// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace cobalt { namespace graphics { namespace testing {

class MenuItem
{
public:
	std::string caption;
	std::function<void()> onClick;
	bool isTickable = false;
	bool isTicked = false;
	bool isSeparator = false;

	MenuItem& EmplaceBack();
	void PushBack(MenuItem&& i);
	MenuItem* Find(const std::string& value);
	void UpdateMenuItem(MenuItem* me, uint32_t* counter);
	std::list<MenuItem>& GetChildren()
	{
		return _subMenuChildren;
	}
	const std::list<MenuItem>& GetChildren() const
	{
		return _subMenuChildren;
	}

#ifdef _WIN32
	void RunCallback(HMENU parentMenu, MenuItem* me, uint32_t commandId);
	HMENU GetHandle();
#endif

private:
#ifdef _WIN32
	HMENU _handle = nullptr;
	uint32_t _commandId = uint32_t(-1);
#endif
	std::list<MenuItem> _subMenuChildren;
};

}}} // namespace cobalt::graphics::testing
