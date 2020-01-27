#include "stdafx.h"

namespace ERROR_UTIL
{
	_NORETURN void Error(const std::string_view errorString)
	{
		std::cout << errorString << std::endl;
		while (7) {}
	}

	_NORETURN void Error(const std::wstring_view errorString)
	{
		std::wcout << errorString << std::endl;
		while (7) {}
	}
}