// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <string>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------------------------------------
// Unicode conversion functions
//----------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
inline std::wstring UTF8ToUTF16(const std::string& stringUTF8)
{
	// Convert the encoding of the supplied string
	std::wstring stringUTF16;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF8.size();
	stringUTF16.reserve(sourceStringSize);
	while (sourceStringPos < sourceStringSize)
	{
		// Determine the number of code units required for the next character
		static const unsigned int codeUnitCountLookup[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 4};
		unsigned int codeUnitCount = codeUnitCountLookup[(unsigned char)stringUTF8[sourceStringPos] >> 4];

		// Ensure that the requested number of code units are left in the source string
		if ((sourceStringPos + codeUnitCount) > sourceStringSize)
		{
			break;
		}

		// Convert the encoding of this character
		switch (codeUnitCount)
		{
		case 1:
		{
			stringUTF16.push_back((wchar_t)stringUTF8[sourceStringPos]);
			break;
		}
		case 2:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x1F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F);
			stringUTF16.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 3:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x0F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F);
			stringUTF16.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 4:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x07) << 18) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 3] & 0x3F);
			wchar_t convertedCodeUnit1 = 0xD800 | (((unicodeCodePoint - 0x10000) >> 10) & 0x03FF);
			wchar_t convertedCodeUnit2 = 0xDC00 | ((unicodeCodePoint - 0x10000) & 0x03FF);
			stringUTF16.push_back(convertedCodeUnit1);
			stringUTF16.push_back(convertedCodeUnit2);
			break;
		}
		}

		// Advance past the converted code units
		sourceStringPos += codeUnitCount;
	}

	// Return the converted string to the caller
	return stringUTF16;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
inline std::string UTF16ToUTF8(const std::wstring& stringUTF16)
{
	// Convert the encoding of the supplied string
	std::string stringUTF8;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF16.size();
	stringUTF8.reserve(sourceStringSize * 2);
	while (sourceStringPos < sourceStringSize)
	{
		// Check if a surrogate pair is used for this character
		bool usesSurrogatePair = (((unsigned int)stringUTF16[sourceStringPos] & 0xF800) == 0xD800);

		// Ensure that the requested number of code units are left in the source string
		if (usesSurrogatePair && ((sourceStringPos + 2) > sourceStringSize))
		{
			break;
		}

		// Decode the character from UTF-16 encoding
		unsigned int unicodeCodePoint;
		if (usesSurrogatePair)
		{
			unicodeCodePoint = 0x10000 + ((((unsigned int)stringUTF16[sourceStringPos] & 0x03FF) << 10) | ((unsigned int)stringUTF16[sourceStringPos + 1] & 0x03FF));
		}
		else
		{
			unicodeCodePoint = (unsigned int)stringUTF16[sourceStringPos];
		}

		// Encode the character into UTF-8 encoding
		if (unicodeCodePoint <= 0x7F)
		{
			stringUTF8.push_back((char)unicodeCodePoint);
		}
		else if (unicodeCodePoint <= 0x07FF)
		{
			char convertedCodeUnit1 = (char)(0xC0 | (unicodeCodePoint >> 6));
			char convertedCodeUnit2 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
		}
		else if (unicodeCodePoint <= 0xFFFF)
		{
			char convertedCodeUnit1 = (char)(0xE0 | (unicodeCodePoint >> 12));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
		}
		else
		{
			char convertedCodeUnit1 = (char)(0xF0 | (unicodeCodePoint >> 18));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 12) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit4 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
			stringUTF8.push_back(convertedCodeUnit4);
		}

		// Advance past the converted code units
		sourceStringPos += (usesSurrogatePair) ? 2 : 1;
	}

	// Return the converted string to the caller
	return stringUTF8;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#ifndef _WIN32
inline std::wstring UTF8ToUTF32(const std::string& stringUTF8)
{
	// Convert the encoding of the supplied string
	std::wstring stringUTF32;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF8.size();
	stringUTF32.reserve(sourceStringSize);
	while (sourceStringPos < sourceStringSize)
	{
		// Determine the number of code units required for the next character
		static const unsigned int codeUnitCountLookup[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 4};
		unsigned int codeUnitCount = codeUnitCountLookup[(unsigned char)stringUTF8[sourceStringPos] >> 4];

		// Ensure that the requested number of code units are left in the source string
		if ((sourceStringPos + codeUnitCount) > sourceStringSize)
		{
			break;
		}

		// Convert the encoding of this character
		switch (codeUnitCount)
		{
		case 1:
		{
			stringUTF32.push_back((wchar_t)stringUTF8[sourceStringPos]);
			break;
		}
		case 2:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x1F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F);
			stringUTF32.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 3:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x0F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F);
			stringUTF32.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 4:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x07) << 18) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 3] & 0x3F);
			stringUTF32.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		}

		// Advance past the converted code units
		sourceStringPos += codeUnitCount;
	}

	// Return the converted string to the caller
	return stringUTF32;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#ifndef _WIN32
inline std::string UTF32ToUTF8(const std::wstring& stringUTF32)
{
	// Convert the encoding of the supplied string
	std::string stringUTF8;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF32.size();
	stringUTF8.reserve(sourceStringSize * 4);
	while (sourceStringPos < sourceStringSize)
	{
		// Each character in the wstring is a unicdoe code point
		auto unicodeCodePoint = (unsigned int)(int)stringUTF32[sourceStringPos];

		// Encode the character into UTF-8 encoding
		if (unicodeCodePoint <= 0x7F)
		{
			stringUTF8.push_back((char)unicodeCodePoint);
		}
		else if (unicodeCodePoint <= 0x07FF)
		{
			char convertedCodeUnit1 = (char)(0xC0 | (unicodeCodePoint >> 6));
			char convertedCodeUnit2 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
		}
		else if (unicodeCodePoint <= 0xFFFF)
		{
			char convertedCodeUnit1 = (char)(0xE0 | (unicodeCodePoint >> 12));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
		}
		else
		{
			char convertedCodeUnit1 = (char)(0xF0 | (unicodeCodePoint >> 18));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 12) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit4 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
			stringUTF8.push_back(convertedCodeUnit4);
		}

		// Move to next character
		++sourceStringPos;
	}

	// Return the converted string to the caller
	return stringUTF8;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
inline std::string NativeWideStringToUTF8(const std::wstring& nativeWideString)
{
#ifdef _WIN32
	return UTF16ToUTF8(nativeWideString);
#else
	return UTF32ToUTF8(nativeWideString);
#endif
}

//----------------------------------------------------------------------------------------------------------------------
inline std::wstring UTF8ToNativeWideString(const std::string& stringUTF8)
{
#ifdef _WIN32
	return UTF8ToUTF16(stringUTF8);
#else
	return UTF8ToUTF32(stringUTF8);
#endif
}

} // namespace cobalt::graphics
