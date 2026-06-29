// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "MenuItem.h"
#include <Cobalt/Debug/Debug.pkg>

namespace cobalt::graphics::testing {

MenuItem& MenuItem::EmplaceBack()
{
	return _subMenuChildren.emplace_back();
}

void MenuItem::PushBack(MenuItem&& i)
{
	_subMenuChildren.push_back(std::move(i));
}

MenuItem* MenuItem::Find(const std::string& value)
{
	for (auto& sub : _subMenuChildren)
	{
		if (sub.caption == value)
		{
			return &sub;
		}
	}
	return nullptr;
}

void MenuItem::UpdateMenuItem(MenuItem* me, uint32_t* counter)
{
	// On POSIX platforms we don't create a native menu bar here, since that
	// would require a GUI toolkit (GTK/Qt/etc.) or custom drawing. We still
	// maintain the logical tree and propagate any bookkeeping via recursion.

	for (auto& subMenu : me->_subMenuChildren)
	{
		subMenu.UpdateMenuItem(&subMenu, counter);
	}

	// If you want to assign synthetic command IDs or do additional
	// bookkeeping for tests here, this is the right place to do it.
	(void)counter;
}

} // namespace cobalt::graphics::testing
