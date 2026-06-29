// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/Debug/Debug.pkg>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
SuccessToken::SuccessToken(bool success)
: _result(success ? 0x01 : 0x00)
{}

//----------------------------------------------------------------------------------------
SuccessToken::SuccessToken(SuccessToken&& source)
: _result(source._result)
{
	source.SetResultObserved();
}

//----------------------------------------------------------------------------------------
SuccessToken::SuccessToken(const SuccessToken& source)
: _result(source._result)
{
	source.SetResultObserved();
}

//----------------------------------------------------------------------------------------
SuccessToken::~SuccessToken()
{
#ifndef COBALT_SUCCESSTOKEN_DISABLE
	if (!IsResultObserved())
	{
#ifdef COBALT_SUCCESSTOKEN_UNOBSERVED_ACTION
		COBALT_SUCCESSTOKEN_UNOBSERVED_ACTION(GetResultInternal());
#else
		ASSERT(0);
#endif
	}
#endif
}

//----------------------------------------------------------------------------------------
// Conversion operators
//----------------------------------------------------------------------------------------
SuccessToken::operator bool() const
{
	return Succeeded();
}

//----------------------------------------------------------------------------------------
// Result methods
//----------------------------------------------------------------------------------------
bool SuccessToken::Succeeded() const
{
	SetResultObserved();
	return GetResultInternal();
}

//----------------------------------------------------------------------------------------
bool SuccessToken::Failed() const
{
	return !Succeeded();
}

//----------------------------------------------------------------------------------------
void SuccessToken::IgnoreResult()
{
	SetResultObserved();
}

//----------------------------------------------------------------------------------------
bool SuccessToken::GetResultInternal() const
{
	return (_result & 0x01) != 0;
}

//----------------------------------------------------------------------------------------
bool SuccessToken::IsResultObserved() const
{
	return (_result & 0x80) != 0;
}

//----------------------------------------------------------------------------------------
void SuccessToken::SetResultObserved() const
{
	_result |= 0x80;
}

}} // namespace cobalt::graphics
