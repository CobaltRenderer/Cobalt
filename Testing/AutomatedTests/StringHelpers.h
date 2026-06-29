// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <string>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
inline bool CaseInsensitiveContains(const std::string& strHaystack, const std::string& strNeedle)
{
	auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(), [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
	return (it != strHaystack.end());
}

//----------------------------------------------------------------------------------------
inline bool CaseInsensitiveContains(const std::wstring& strHaystack, const std::string& strNeedle)
{
	auto it = std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(), strNeedle.end(), [](wchar_t ch1, wchar_t ch2) { return std::towupper(ch1) == std::towupper(ch2); });
	return (it != strHaystack.end());
}

//----------------------------------------------------------------------------------------
inline std::string StringReplaceAll(std::string s, const std::string& from, const std::string& to)
{
	if (!from.empty())
	{
		std::size_t pos = 0;
		while ((pos = s.find(from, pos)) != std::string::npos)
		{
			s.replace(pos, from.size(), to);
			pos += to.size();
		}
	}
	return s;
}

} // namespace cobalt::graphics::testing
