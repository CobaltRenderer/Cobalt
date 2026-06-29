// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "MenuItem.h"
#include <Cobalt/Debug/Debug.pkg>
#include "UnicodeConversion.h"
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
MenuItem& MenuItem::EmplaceBack()
{
	return _subMenuChildren.emplace_back();
}

//----------------------------------------------------------------------------------------
void MenuItem::PushBack(MenuItem&& i)
{
	_subMenuChildren.push_back(std::move(i));
}

//----------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------
void MenuItem::UpdateMenuItem(MenuItem* me, uint32_t* counter)
{
	for (auto& subMenu : me->_subMenuChildren)
	{
		subMenu.UpdateMenuItem(&subMenu, counter);
	}

	if (_handle != nullptr)
	{
		DestroyMenu(_handle);
		_handle = nullptr;
	}

	if (!me->_subMenuChildren.empty())
	{
		_handle = CreateMenu();
		ASSERT(_handle != nullptr);
	}
	if (_handle == nullptr)
	{
		return;
	}

	for (auto& subMenu : me->_subMenuChildren)
	{
		if (!subMenu._subMenuChildren.empty())
		{
			ASSERT(subMenu._handle != nullptr);
			_commandId = 0;

			AppendMenuW(_handle, MF_STRING | MF_POPUP, (UINT_PTR)subMenu._handle, UTF8ToUTF16(subMenu.caption).c_str());
		}
		else if (subMenu.isSeparator)
		{
			AppendMenuW(_handle, MF_SEPARATOR, 0, nullptr);
		}
		else
		{
			subMenu._commandId = (*counter)++;
			AppendMenuW(_handle, MF_STRING | (subMenu.isTickable ? (subMenu.isTicked ? MF_CHECKED : MF_UNCHECKED) : 0), subMenu._commandId, UTF8ToUTF16(subMenu.caption).c_str());
		}
	}
}

//----------------------------------------------------------------------------------------
void MenuItem::RunCallback(HMENU parentMenu, MenuItem* me, uint32_t commandId)
{
	if (!me->_subMenuChildren.empty())
	{
		for (auto& subMenu : me->_subMenuChildren)
		{
			subMenu.RunCallback((subMenu._handle != nullptr ? subMenu._handle : parentMenu), &subMenu, commandId);
		}
	}
	else if (_commandId == commandId)
	{
		if (me->isTickable)
		{
			me->isTicked = !me->isTicked;
			CheckMenuItem(parentMenu, me->_commandId, me->isTicked ? MF_CHECKED : MF_UNCHECKED);
		}

		if (me->onClick)
		{
			me->onClick();
		}
	}
}

//----------------------------------------------------------------------------------------
HMENU MenuItem::GetHandle()
{
	return _handle;
}

} // namespace cobalt::graphics::testing
