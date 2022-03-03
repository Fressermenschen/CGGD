#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <comdef.h>
#include <stdexcept>
#include <string>
#include <windows.h>

#define THROW_ERROR(x)                            \
	{                                             \
		std::string message(x);                   \
		message.append(" (at ")                   \
				.append(__FILE__)                 \
				.append(":")                      \
				.append(std::to_string(__LINE__)) \
				.append(")")                      \
				.append("\n");                    \
		throw std::runtime_error(message);        \
	}

inline void ThrowIfFailed(const HRESULT hr)
{
	if (FAILED(hr))
	{
		LPTSTR errorText = nullptr;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
					  nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&errorText),
					  0, nullptr);

		if (errorText)
		{
			MessageBox(nullptr, errorText, TEXT("Runtime failure"), MB_ICONERROR);
			LocalFree(errorText);
			THROW_ERROR("Unhandled Exception")
		}
	}
}